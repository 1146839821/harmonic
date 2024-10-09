#include <msg.h>
#include <utils.h>
#include <tlzma.h>

#include <ace.h>

HMODULE ace_load_shell_module() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, sizeof(path));
    
    wchar_t *extensionDot = wcsrchr(path, L'.');
    wcscpy(extensionDot, L"Base.dll");

    return utils_load_module_patched(path);
}

HMODULE ace_load_base_module(void *init3Addr) {
    HMODULE base = utils_load_module_patched(L"AntiCheatExpert\\InGame\\x64\\ACE-Base64.dll");
    
    void *init3 = GetProcAddress(base, "InitAceClient3");
    utils_hook_address(init3, init3Addr);
    
    return base;
}

HMODULE ace_load_driver_module(void *coAddr) {
    HMODULE base = utils_load_module_patched(L"AntiCheatExpert\\InGame\\x64\\ACE-DRV64.dll");
    
    void *co = GetProcAddress(base, "CreateObject");
    utils_hook_address(co, coAddr);
    
    return base;
}

static void ace_unpack_section(void *data, size_t size) {
    void *packed = malloc(size);
    memcpy(packed, data, size);

    uint8_t *unpackedPtr = (uint8_t*)data;
    uint8_t *packedPtr = (uint8_t*)packed;

    while (tlzma_test_header(packedPtr)) {
        size_t unpackedSize, packedSize;
        tlzma_decode(unpackedPtr, packedPtr, &unpackedSize, &packedSize);

        unpackedPtr += unpackedSize;
        packedPtr += packedSize;
    }

    free(packed);
}

void ace_unpack_sections(HMODULE module) {
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)module;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(((uint8_t*)module) + dosHeader->e_lfanew);

    WORD sectionCount = ntHeaders->FileHeader.NumberOfSections;
    IMAGE_SECTION_HEADER* sectionHeaders = (IMAGE_SECTION_HEADER*)(ntHeaders + 1);

    for (WORD i = 0; i < sectionCount; i++) {
        uint8_t *data = ((uint8_t*)module) + sectionHeaders[i].VirtualAddress;

        if (tlzma_test_header(data)) {
            MEMORY_BASIC_INFORMATION memoryInfo;
            VirtualQuery(data, &memoryInfo, sizeof(memoryInfo));
            
            DWORD oldProtect;
            VirtualProtect(data, memoryInfo.RegionSize, PAGE_READWRITE, &oldProtect);

            ace_unpack_section(data, memoryInfo.RegionSize);

            VirtualProtect(data, memoryInfo.RegionSize, oldProtect, &oldProtect);
        }
    }
}
