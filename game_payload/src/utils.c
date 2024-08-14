#include <windows.h>
#include <tlhelp32.h>

#include <msg.h>
#include <crc32c.h>
#include <ntdll.h>

#include <utils.h>

void utils_map_file(const wchar_t *path, struct file_mapping *map) {
    map->file = CreateFileW(path, FILE_READ_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (map->file == INVALID_HANDLE_VALUE) {
        msg_err_w(L"Could not open file: %ls", path);
    }

    map->mapping = CreateFileMappingA(map->file, NULL, PAGE_READONLY, 0, 0, NULL);
    map->data = MapViewOfFile(map->mapping, FILE_MAP_READ, 0, 0, 0);
    if (!map->data) {
        msg_err_w(L"Could not map view of file %ls", path);
    }
}

void utils_unmap_file(struct file_mapping *map)  {
    UnmapViewOfFile(map->data);
    CloseHandle(map->mapping);
    CloseHandle(map->file);
}

int utils_path_exists(const wchar_t *filePath) {
    return GetFileAttributesW(filePath) != INVALID_FILE_ATTRIBUTES;
}

uint32_t utils_file_crc32c(const wchar_t *filePath) {
    struct file_mapping map;
    utils_map_file(filePath, &map);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(map.file, &fileSize);

    uint32_t crc = crc32c(0, map.data, fileSize.QuadPart);

    utils_unmap_file(&map);
    return crc;
}

// https://stackoverflow.com/a/16719260
void utils_create_parent_dirs(const wchar_t *path) {
    wchar_t dir[MAX_PATH];
    ZeroMemory(dir, sizeof(dir));

    const wchar_t *end = path - 1;

    while((end = wcschr(++end, L'\\')) != NULL) {
        wcsncpy(dir, path, end - path + 1);

        if (!utils_path_exists(dir) && !CreateDirectoryW(dir, NULL)) {
            msg_err_w(L"Failed to create directory: %ls", dir);
        }
    }
}

void utils_save_to_file(const wchar_t *filePath, const void *buf, size_t length) {
    HANDLE file = CreateFileW(filePath, FILE_WRITE_ACCESS, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        msg_err_w(L"Could not create file: %s", filePath);
    }

    WriteFile(file, buf, length, NULL, FALSE);

    CloseHandle(file);
}

char utils_env_enabled(const char *env) {
    char *envText = getenv(env);
    return envText && *envText;
}

void utils_write_protected_memory(void *addr, const void *buf, size_t size) {
    DWORD oldProtect;
    VirtualProtect(addr, size, PAGE_READWRITE, &oldProtect);

    memcpy(addr, buf, size);

    VirtualProtect(addr, size, oldProtect, &oldProtect);
}

void utils_suspend_other_threads() {
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(threadEntry);
    if (!Thread32First(handle, &threadEntry)) {
        msg_err_a("Failed to enumerate threads");
    }

    do {
        if (threadEntry.dwSize < FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(threadEntry.th32OwnerProcessID)) {
            threadEntry.dwSize = sizeof(threadEntry);
            continue;
        }

        if(threadEntry.th32ThreadID != GetCurrentThreadId() && threadEntry.th32OwnerProcessID == GetCurrentProcessId())
        {
            HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadEntry.th32ThreadID);
            if (thread == NULL) {
                continue;
            }
            SuspendThread(thread);
            CloseHandle(thread);
        }

        threadEntry.dwSize = sizeof(threadEntry);
    } while (Thread32Next(handle, &threadEntry));

    CloseHandle(handle);
}

static void dll_notification(ULONG reason, const PLDR_DLL_NOTIFICATION_DATA data, void *context) {
    if (reason != 1) { // 1 - attach
        return;
    }

    // context should be set to the target module name
    wchar_t *targetModuleName = (wchar_t*)context;

    if (wcsicmp(targetModuleName, data->Loaded.BaseDllName->Buffer) != 0) {
        return;
    }
    
    char *cModule = (char*)data->Loaded.DllBase;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)cModule;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(cModule + dosHeader->e_lfanew);

    // Replace entry point with a stub
    char *entryPoint = cModule + ntHeaders->OptionalHeader.AddressOfEntryPoint;
    
    const char ENTRY_POINT_STUB[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00,   // mov eax, 1
        0xC3                            // ret
    };
    
    utils_write_protected_memory(entryPoint, ENTRY_POINT_STUB, sizeof(ENTRY_POINT_STUB));
    
    // Replace exports with a stub
    uint32_t exportsOffset = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    IMAGE_EXPORT_DIRECTORY *exports = (IMAGE_EXPORT_DIRECTORY*)(cModule + exportsOffset);
    uint32_t *functions = (uint32_t*)(cModule + exports->AddressOfFunctions);
    
    const char EXPORT_STUB[] = {
        0xB8, 0x00, 0x00, 0x00, 0x00,   // mov eax, 0
        0xC3                            // ret
    };
    
    for (size_t i = 0; i < exports->NumberOfFunctions; i++) {
        char *functionAddr = cModule + functions[i];
        utils_write_protected_memory(functionAddr, EXPORT_STUB, sizeof(EXPORT_STUB));
    }

    // Break the TLS struct
    IMAGE_DATA_DIRECTORY *tlsDirectory = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    
    IMAGE_DATA_DIRECTORY zero = { 0 };
    utils_write_protected_memory(tlsDirectory, &zero, sizeof(zero));
}

HMODULE utils_load_module_patched(wchar_t *path) {
    // Get filename from the path
    wchar_t *name = wcsrchr(path, '\\');
    name = name ? name + 1 : path;

    void *cookie;
    LdrRegisterDllNotification(0, &dll_notification, name, &cookie);

    HMODULE module = LoadLibraryW(path);
    if (!module) {
        msg_err_w(L"Could not load module: %ls", path);
    }

    // LoadLibraryW is synchronous; the notification function has already finished executing
    LdrUnregisterDllNotification(cookie);

    return module;
}

void utils_hook_address(void *addr, void *target) {
    char hook[] = {
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp [$ + 6]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    *((void**)(hook + 6)) = target;

    utils_write_protected_memory(addr, hook, sizeof(hook));
}
