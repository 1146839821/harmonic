#include <stdio.h>

#include <inject.h>
#include <envs.h>

#include <game_p.h>

int wmain(int argc, wchar_t **argv) {
    // Read arguments
    if (argc != 3) {
        wprintf(L"Usage: harmonic_dumper.exe [game path] [out dump path]\n");
        return 0;
    }

    wchar_t *gamePath = argv[1];
    wchar_t *dumpPath = argv[2];

    // Compute absolute arg paths
    wchar_t gameExePath[MAX_PATH];
    GetFullPathNameW(gamePath, MAX_PATH, gameExePath, NULL);

    wchar_t dumpFilePath[MAX_PATH];
    GetFullPathNameW(dumpPath, MAX_PATH, dumpFilePath, NULL);

    // cd into the injector directory
    wchar_t injectorPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(NULL), injectorPath, MAX_PATH);

    *(wcsrchr(injectorPath, L'\\')) = L'\0';

    SetCurrentDirectoryW(injectorPath);

    // Compute absolute dll path
    wchar_t gamePayloadPath[MAX_PATH];
    GetFullPathNameW(GAME_INJECT_DLL, MAX_PATH, gamePayloadPath, NULL);

    // Compute the working directory path
    wchar_t workdir[MAX_PATH];
    wcscpy(workdir, gameExePath);
    *(wcsrchr(workdir, L'\\')) = L'\0';

    // Set evnvars
    SetEnvironmentVariableW(L"HARMONIC_DUMP_FILE", dumpFilePath);
    SetEnvironmentVariableW(L"HARMONIC_MODE", L"dumper");

    // Start the game
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(
        gameExePath,
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        workdir,
        &si,
        &pi
    )) {
        fwprintf(stderr, L"Could not start process! (%ld)\n", GetLastError());
        exit(1);
    }

    // Inject
    void *payloadStart = &_binary_game_p_o_p_game_p_bin_start;
    size_t payloadSize = (size_t)&_binary_game_p_o_p_game_p_bin_size;
    inject(pi.hProcess, payloadStart, payloadSize, gamePayloadPath, 0);

    // Optional: wait for user input before resuming (useful for debugging) 
    char *waitEnabled = getenv("WAIT_BEFORE_RESUME");
    if (waitEnabled && *waitEnabled) {
        wprintf(L"PID: %ld. Press any button to continue\n", pi.dwProcessId);
        fgetc(stdin);
    }

    // Resume the process
    ResumeThread(pi.hThread);
    
    wprintf(L"Started dumper\n");

    return 0;
}
