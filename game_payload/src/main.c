#include <windows.h>

#include <core.h>
#include <utils.h>
#include <msg.h>
#include <pe.h>
#include <crc32c.h>
#include <ace.h>
#include <config.h>

#define NTDLL_DYNAMIC_LINK_IMPL
#include <ntdll.h>

#include <main.h>

enum harmonic_mode {
    MODE_LOADER,
    MODE_DUMPER
};

HMODULE this_module;
enum harmonic_mode mode;
void *oep;

wchar_t *sDumpPath;

#pragma pack(push, 1)
struct dump_metadata {
    uint32_t version_len;
    uint32_t game_crc32c;
    uint32_t dump_crc32c;
};
#pragma pack(pop)

static void load(const wchar_t *dumpPath) {
    if (!utils_path_exists(dumpPath)) {
        msg_err_w(L"Invalid dump file path: %s", dumpPath);
    }

    // Load dump
    struct file_mapping map;
    utils_map_file(dumpPath, &map);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(map.file, &fileSize);

    // Verify metadata
    if (fileSize.QuadPart < sizeof(struct dump_metadata)) {
        msg_err_a("'%s' is not a valid dump file", dumpPath);
    }

    size_t metadataOffset = fileSize.QuadPart - sizeof(struct dump_metadata);
    struct dump_metadata *metadata = (struct dump_metadata*)(map.data + metadataOffset);    
    
    size_t dumpSize = metadataOffset - metadata->version_len;

    // Checksums
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, sizeof(exePath));
    if (metadata->game_crc32c != utils_file_crc32c(exePath)) {
        msg_err_a("Game checksum mismatch, dump is likely outdated or corrupt and a new one is required");
    }

    if (metadata->dump_crc32c != crc32c(0, map.data, dumpSize)) {
        msg_err_a("Dump checksum mismath, dump file is corrupt");
    }

    // Version check
    char *versionStr = malloc(metadata->version_len + 1); 
    memcpy(versionStr, map.data + dumpSize, metadata->version_len);
    versionStr[metadata->version_len] = '\0';

    char *versionSep = strchr(versionStr, '_');
    if (versionSep) {
        // decoupled format, check dump version
        if (strcmp(versionSep + 1, DUMP_VERSION) != 0) {
            *versionSep = '\0';
            msg_err_a("Version mismatch, dump was created by v%s (dump v%s), but loader is v" HARMONIC_VERSION " (dump v" DUMP_VERSION ")", versionStr, versionSep + 1);
        }
    } else {
        // old format
        msg_err_a("Version mismatch, dump was created by v%s, but loader is v" HARMONIC_VERSION, versionStr);
    }

    free(versionStr);

    // Unpack sections
    HMODULE exeModule = GetModuleHandleA(NULL);

    LARGE_INTEGER frequency, unpackStart, unpackEnd;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&unpackStart);

    ace_unpack_sections(exeModule);

    QueryPerformanceCounter(&unpackEnd);
    if (utils_env_enabled("HARMONIC_UNPACK_TIME")) {
        uint64_t elapsed = unpackEnd.QuadPart - unpackStart.QuadPart;
        double lfElapsed = (double)elapsed / (double)frequency.QuadPart;
        msg_info_a("Unpacked in %.3lf seconds (%llu QPC ticks)", lfElapsed, elapsed);
    }

    // Load
    core_do_load((char*)map.data, dumpSize, &oep);

    utils_unmap_file(&map);

    // Handle the other AC part
    ace_load_shell_module();

    if (!utils_env_enabled("HARMONIC_NO_AC")) {
        ace_load_driver_module(&core_drv_co);
    } else {
        msg_info_a("Enabled full no-anticheat mode. You will likely experience periodic disconnects");
        ace_load_base_module(&core_ace_init);
    }
}

static void dump() {
    utils_suspend_other_threads();

    size_t dumpSize;
    char *buf = core_do_dump(&dumpSize);
    
    // Add metadata to the dump
    char *versionStr = HARMONIC_VERSION "_" DUMP_VERSION;
    uint32_t versionLen = strlen(versionStr);
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, sizeof(exePath));
    
    size_t fileSize = dumpSize + versionLen + sizeof(struct dump_metadata);
    buf = realloc(buf, fileSize);
    
    memcpy(buf + dumpSize, versionStr, versionLen);

    struct dump_metadata *metadata = (struct dump_metadata*)(buf + dumpSize + versionLen);
    metadata->version_len = versionLen;
    metadata->game_crc32c = utils_file_crc32c(exePath);
    metadata->dump_crc32c = crc32c(0, buf, dumpSize);
    
    // Save dump
    utils_save_to_file(sDumpPath, buf, fileSize);
    free(buf);
    
    msg_info_w(L"Successfully dumped data to %s", sDumpPath);
    free(sDumpPath);
    exit(0);
}

static void fallback_dump() {
    msg_warn_a("CreateDXGIFactory hook did not get called. Running from fallback. You can ignore this message");
    dump();
}

static void setup_dumper(const wchar_t *dumpPath) {
    sDumpPath = malloc((wcslen(dumpPath) + 1) * sizeof(wchar_t));
    wcscpy(sDumpPath, dumpPath);
   
    HMODULE dxgi = LoadLibraryA("dxgi.dll");

    const char *hookFunctions[] = {
        "CreateDXGIFactory",
        "CreateDXGIFactory1",
        "CreateDXGIFactory2"
    };
    for (size_t i = 0; i < UTILS_COUNT(hookFunctions); i++) {
        utils_hook_address(GetProcAddress(dxgi, hookFunctions[i]), &dump);
    }

    // Fallback in case the CreateDXGIFactory hook did not get called
    ace_load_base_module(&fallback_dump);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    // Only listen to attach
    if (reason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }
    
    // Dynamically link functions from ntdll
    ntdll_link();

    // Make CrashSight64 go away
    utils_load_module_patched(L"CrashSight64.dll");

    // Bugtrace can go too
    utils_load_module_patched(L"AntiCheatExpert\\InGame\\x64\\ACE-Trace.dll");

     utils_load_module_patched(L"AntiCheatExpert\\InGame\\x64\\ACE-Safe.dll");

    this_module = instance;

    wchar_t dumpPath[MAX_PATH];
    GetEnvironmentVariableW(L"HARMONIC_DUMP_FILE", dumpPath, sizeof(dumpPath));

    char *modeStr = getenv("HARMONIC_MODE");
    if (!modeStr) {
        msg_err_a("Injector did not pass mode. " ISSUE_SUFFIX);
    }

    if (strcmp(modeStr, "loader") == 0) {
        mode = MODE_LOADER;
        load(dumpPath);
    } else if (strcmp(modeStr, "dumper") == 0) {
        mode = MODE_DUMPER;
        setup_dumper(dumpPath);
    } else {
        msg_err_a("Injector passed invalid mode: '%s'. " ISSUE_SUFFIX, modeStr);
    }

    return TRUE;
}

void *get_entry_addr() {
    switch (mode) {
    case MODE_LOADER:
        return oep; 
    case MODE_DUMPER:
        return pe_find_entry_point(GetModuleHandleA(NULL));
    default:
        return NULL;
    }
}
