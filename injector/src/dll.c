#include <stdio.h>

#include <inject.h>
#include <envs.h>

#include <game_p.h>

typedef char *(*wgufn_t)(wchar_t* path); // wine_get_unix_file_name

const wchar_t *H_MB_TITLE = L"Harmonic Launcher Payload";

static int str_starts_with(char *str, char *prefix) {
    while (*str && *prefix) {
        if (*str != *prefix) {
            return 0;
        }
        
        str++, prefix++;
    }

    if (*prefix) {
        return 0;
    }

    return 1;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    // Only listen for attach
    if (reason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    // Get target EXE path
    wchar_t targetExe[MAX_PATH];
    GetEnvironmentVariableW(ENV_EXE_PATH, targetExe, MAX_PATH);

    // Get the path of the DLL to inject
    wchar_t injectDll[MAX_PATH];
    GetEnvironmentVariableW(ENV_DLL_PATH, injectDll, MAX_PATH);

    // Get the path of the dump file
    wchar_t dumpFile[MAX_PATH];
    GetEnvironmentVariableW(L"HARMONIC_DUMP_FILE", dumpFile, MAX_PATH);

    // Get game commandline
    wchar_t cmdline[8192];
    GetEnvironmentVariableW(ENV_PROC_CMD, cmdline, sizeof(cmdline) / sizeof(wchar_t));

    // Compute the working directory path
    wchar_t workdir[MAX_PATH];
    wcscpy(workdir, targetExe);
    *(wcsrchr(workdir, L'\\')) = L'\0';

    // Compute the path to the top level of the game directory (should be 4 layers up)
    wchar_t gameDir[MAX_PATH];
    wcscpy(gameDir, targetExe);

    wchar_t *sep;
    for (size_t i = 0; i < 4; i++) {
        sep = wcsrchr(gameDir, L'\\');
        if (!sep) {
            break;
        }
        *sep = L'\0';
    }

    if (!sep) {
        wchar_t message[128];
        wsprintfW(message, L"Game directory path could not be determined from executable path: '%s'", targetExe);
        MessageBoxW(NULL, message, H_MB_TITLE, MB_OK | MB_ICONERROR);
        exit(1);
    }

    // SAFETY: verify that the injector and the dump are not inside the game directory
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    wgufn_t wine_get_unix_file_name = (wgufn_t)GetProcAddress(kernel32, "wine_get_unix_file_name");

    if (wine_get_unix_file_name) {
        char *unixInjectDll = wine_get_unix_file_name(injectDll);
        if (!unixInjectDll) {
            MessageBoxW(NULL, L"Invalid DLL path (internal injector error). Please open an issue on the harmonic repository", H_MB_TITLE, MB_OK | MB_ICONERROR);
            exit(1);
        }

        char *unixGameDir = wine_get_unix_file_name(gameDir);
        if (!unixGameDir) {
            wchar_t message[MAX_PATH + 64];
            wsprintfW(message, L"Invalid target executable path: '%s'", targetExe);
            MessageBoxW(NULL, message, H_MB_TITLE, MB_OK | MB_ICONERROR);
            exit(1);
        }

        char *unixDumpFile = wine_get_unix_file_name(dumpFile);
        if (!unixDumpFile) {
            wchar_t message[MAX_PATH + 64];
            wsprintfW(message, L"Invalid target dump path: '%s'", dumpFile);
            MessageBoxW(NULL, message, H_MB_TITLE, MB_OK | MB_ICONERROR);
            exit(1);
        }

        if (str_starts_with(unixInjectDll, unixGameDir)) {
            MessageBoxW(NULL, L"Putting the loader (or any other foreign PE binaries) inside the game directory is dangerous! Please move it elsewhere.", H_MB_TITLE, MB_OK | MB_ICONERROR);
            exit(1);
        }

        if (str_starts_with(unixDumpFile, unixGameDir)) {
            MessageBoxW(NULL, L"Putting the dump file in the game directory is potentially dangerous. Please move it elsewhere.", H_MB_TITLE, MB_OK | MB_ICONERROR);
            exit(1);
        }

        HANDLE heap = GetProcessHeap();
        HeapFree(heap, 0, unixInjectDll);
        HeapFree(heap, 0, unixGameDir);
        HeapFree(heap, 0, unixDumpFile);
    } else {
        MessageBoxW(NULL, L"Could not find wine_get_unix_file_name! Wine version too old?", H_MB_TITLE, MB_OK | MB_ICONWARNING);
    }

    // Start the game
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(
        NULL,
        cmdline,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        workdir,
        &si,
        &pi
    )) {
        wchar_t message[1024];
        wsprintfW(message, L"Failed to start game process: %ld\nGame executable path: '%ls'", GetLastError(), targetExe);
        MessageBoxW(NULL, message, H_MB_TITLE, MB_OK | MB_ICONERROR);

        exit(1);
    }

    // Inject
    void *payloadStart = &_binary_game_p_o_p_game_p_bin_start;
    size_t payloadSize = (size_t)&_binary_game_p_o_p_game_p_bin_size;
    inject(pi.hProcess, payloadStart, payloadSize, injectDll, 1);

    // Optional: wait for user input before resuming (useful for debugging)
    char *waitEnabled = getenv("WAIT_BEFORE_RESUME");
    if (waitEnabled && *waitEnabled) {
        wchar_t message[64];
        wsprintfW(message, L"PID: %ld. Press OK to continue", pi.dwProcessId);
        MessageBoxW(NULL, message, H_MB_TITLE, MB_OK | MB_ICONINFORMATION);
    }

    // Resume the process
    ResumeThread(pi.hThread);

    // The launcher process should now hang untill the game terminates
    WaitForSingleObject(pi.hProcess, INFINITE);

    return TRUE;
}
