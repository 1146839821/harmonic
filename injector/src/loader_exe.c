#include <stdio.h>

#include <inject.h>
#include <envs.h>

#include <launcher_p.h>

#define SHIFT(argc, argv) argc--, argv++

int wmain(int argc, wchar_t **argv) {
    // Read arguments
    wchar_t *gamePath = NULL;
    wchar_t *launcherPath = NULL;
    wchar_t *dumpPath = NULL;

    // Skip executable
    SHIFT(argc, argv);
    
    switch (argc) {
        case 0:
        case 1:
            wprintf(L"Usage: harmonic_loader.exe [game path] [dump path] <launcher path>\n");
            return 0;
        case 2:
            gamePath = argv[0];
            SHIFT(argc, argv);

            dumpPath = argv[0];
            SHIFT(argc, argv);

            launcherPath = L"--";

            break;
        default:
            gamePath = argv[0];
            SHIFT(argc, argv);

            dumpPath = argv[0];
            SHIFT(argc, argv);

            launcherPath = argv[0];
            SHIFT(argc, argv);

            break;
    }

    // Default launcher path
    if (wcscmp(launcherPath, L"--") == 0) {
        wprintf(L"No launcher process specified! Using explorer.exe\n");
        launcherPath = L"C:\\Windows\\explorer.exe";
    } 

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

    // Compute absolute dll paths
    wchar_t gamePayloadPath[MAX_PATH];
    GetFullPathNameW(GAME_INJECT_DLL, MAX_PATH, gamePayloadPath, NULL);

    wchar_t launcherPayloadPath[MAX_PATH];
    GetFullPathNameW(LAUNCHER_INJECT_DLL, MAX_PATH, launcherPayloadPath, NULL);

    // Construct commandline for the game process
    wchar_t cmdline[8192];
    wsprintfW(cmdline, L"\"%ls\"", gameExePath);

    while (argc) {
        wchar_t arg[8192];
        wsprintfW(arg, L" \"%ls\"", argv[0]);
        wcscat(cmdline, arg);

        SHIFT(argc, argv);
    }

    // Set envvars
    SetEnvironmentVariableW(ENV_EXE_PATH, gameExePath);
    SetEnvironmentVariableW(ENV_DLL_PATH, gamePayloadPath);
    SetEnvironmentVariableW(ENV_PROC_CMD, cmdline);

    SetEnvironmentVariableW(L"HARMONIC_DUMP_FILE", dumpFilePath);
    SetEnvironmentVariableW(L"HARMONIC_MODE", L"loader");

    // Start the launcher
    wprintf(L"Starting '%ls' via '%ls' with '%ls'\n", gameExePath, launcherPath, dumpFilePath);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(
        launcherPath,
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        fwprintf(stderr, L"Could not start process! (%ld)\n", GetLastError());
        exit(1);
    }

    wprintf(L"Started launcher process (%ld)\n", pi.dwProcessId);

    // Inject
    void *payloadStart = &_binary_launcher_p_o_p_launcher_p_bin_start;
    size_t payloadSize = (size_t)&_binary_launcher_p_o_p_launcher_p_bin_size; // yes this is valid
    inject(pi.hProcess, payloadStart, payloadSize, launcherPayloadPath, 1);

    // Resume the process
    ResumeThread(pi.hThread);

    return 0;
}
