#include <stdint.h>

#include <inject.h>

#define TRAMPOLINE_SIZE (6 + sizeof(void*))
#define ENTRY_SIZE 5

// Original values to recover after the injection
// Recovery is performed by the assembly payload
#pragma pack(push, 1)
struct recovery_data {
    void *trampolineAddress;
    char trampolineData[TRAMPOLINE_SIZE];

    void *entryPointAddress;
    char entryPointData[ENTRY_SIZE];

    void *importDescriptorAddress;
    IMAGE_IMPORT_DESCRIPTOR importDescriptorData;

    void *sizeFieldAddress;
    DWORD sizeFieldData;
};
#pragma pack(pop)

static inline void write_protected_process_memory(HANDLE process, void *address, const void *buf, size_t size) {
    DWORD oldProtect;
    VirtualProtectEx(process, address, size, PAGE_EXECUTE_READWRITE, &oldProtect);

    WriteProcessMemory(process, address, buf, size, NULL);

    VirtualProtectEx(process, address, size, oldProtect, &oldProtect);
}

void inject(HANDLE process, const void *payload, size_t payloadSize, const wchar_t *dllPath, char breakImports) {
    // Find the EXE header in the process
    char exeHeader[1024];
    IMAGE_DOS_HEADER *dosHeader = NULL;
    IMAGE_NT_HEADERS64 *ntHeaders = NULL;

    MEMORY_BASIC_INFORMATION memoryInfo;
    char *currentAddress = 0x0;
    while (VirtualQueryEx(process, currentAddress, &memoryInfo, sizeof(memoryInfo))) {
        ReadProcessMemory(process, currentAddress, exeHeader, sizeof(exeHeader), NULL);

        dosHeader = (IMAGE_DOS_HEADER*)exeHeader;

        // DOS header magic "MZ"
        if (dosHeader->e_magic != 0x5A4D) {
            goto cont;
        } 

        ntHeaders = (IMAGE_NT_HEADERS64*)(exeHeader + dosHeader->e_lfanew);

        // NT header signature "PE"
        if (ntHeaders->Signature != 0x4550) {
            goto cont;
        }

        // Skip DLLs
        if ((ntHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL) {
            goto cont;
        }

        // Skip potential headers without an entry point 
        // I have no idea how and why they exist, but apparently they do
        if (ntHeaders->OptionalHeader.AddressOfEntryPoint == 0) {
            goto cont;
        }

        // Found EXE header
        break;

    cont:
        currentAddress += memoryInfo.RegionSize;
    }

    char *exe = (char*)memoryInfo.BaseAddress;


    // Inject the loader into the process
    size_t dllPathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);

    size_t allocSize = payloadSize + dllPathSize + sizeof(struct recovery_data);
    char *remoteAlloc = VirtualAllocEx(process, NULL, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // Write the assembly payload and dll path
    WriteProcessMemory(process, remoteAlloc, payload, payloadSize, NULL);
    WriteProcessMemory(process, remoteAlloc + payloadSize, dllPath, dllPathSize, NULL);


    // Modify the executable to run the assembly payload
    // Recovery data structure
    struct recovery_data rd;

    // Find a place for the trampoline to the assembly payload
    char *trampoline = NULL;
    IMAGE_SECTION_HEADER *sections = (IMAGE_SECTION_HEADER*)(ntHeaders + 1);
    for (size_t i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        if (sections[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            trampoline = exe + sections[i].VirtualAddress;
        }
    }

    // Save original trampoline bytes
    rd.trampolineAddress = trampoline;
    ReadProcessMemory(process, rd.trampolineAddress, rd.trampolineData, sizeof(rd.trampolineData), NULL);

    // Write the trampoline
    char trampolinePayload[TRAMPOLINE_SIZE] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00 };
    memcpy(trampolinePayload + 6, &remoteAlloc, sizeof(remoteAlloc));

    write_protected_process_memory(process, trampoline, trampolinePayload, sizeof(trampolinePayload));

    // Save original entry point bytes
    char *entryPoint = exe + ntHeaders->OptionalHeader.AddressOfEntryPoint;

    rd.entryPointAddress = entryPoint;
    ReadProcessMemory(process, rd.entryPointAddress, rd.entryPointData, sizeof(rd.entryPointData), NULL);

    // Replace the entry point with a jump to the trampoline
    int32_t trampolineOffset = (int32_t)((ptrdiff_t)trampoline - (ptrdiff_t)entryPoint - 5);

    char entryPayload[ENTRY_SIZE] = { 0xE9 };
    memcpy(entryPayload + 1, &trampolineOffset, sizeof(trampolineOffset));
    
    write_protected_process_memory(process, entryPoint, entryPayload, sizeof(entryPayload));


    // Break the import table to prevent any dlls from being loaded if break_imports is set
    // Step 1: break the first import descriptor
    char *importDescriptors = exe + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    // Save the original descriptor address and bytes
    rd.importDescriptorAddress = importDescriptors;
    ReadProcessMemory(process, rd.importDescriptorAddress, &rd.importDescriptorData, sizeof(rd.importDescriptorData), NULL);

    // Overwrite with zeroes    
    if (breakImports) {
        IMAGE_IMPORT_DESCRIPTOR firstDescriptor;
        ZeroMemory(&firstDescriptor, sizeof(firstDescriptor));
        write_protected_process_memory(process, importDescriptors, &firstDescriptor, sizeof(firstDescriptor));
    }

    // Step 2: break the image data directory entry
    char* ddAddr = ((char*)&(ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)) - exeHeader + exe;
    
    // Save the original value
    rd.sizeFieldAddress = ddAddr;
    ReadProcessMemory(process, rd.sizeFieldAddress, &rd.sizeFieldData, sizeof(rd.sizeFieldData), NULL);

    // Set to 0
    if (breakImports) {
        DWORD newSize = 0;
        write_protected_process_memory(process, ddAddr, &newSize, sizeof(newSize));
    }

    // Write recovery data to the allocation
    WriteProcessMemory(process, remoteAlloc + payloadSize + dllPathSize, &rd, sizeof(rd), NULL);
}
