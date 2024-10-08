#include <windows.h>
#include <stdio.h>
#include <config.h>

#include <msg.h>

#define DEF_MSG_FN(name, type, printfn, mbfn, projname, flags, suffix) \
    void name(const type *format, ...) { \
        va_list args; \
        va_start(args, format); \
        \
        int count = printfn(NULL, 0, format, args) + 1; \
        \
        type *buf = malloc(count * sizeof(type)); \
        printfn(buf, count, format, args); \
        \
        mbfn(NULL, buf, projname, flags); \
        \
        va_end(args); \
        \
        free(buf); \
        suffix; \
    }

const char    *TITLE_A = "v" HARMONIC_VERSION " Harmonic";
const wchar_t *TITLE_W = L"v" HARMONIC_VERSION " Harmonic";

// Error
DEF_MSG_FN(msg_err_a, char, _vsnprintf, MessageBoxA, TITLE_A, MB_OK | MB_ICONERROR, exit(1))
DEF_MSG_FN(msg_err_w, wchar_t, _vsnwprintf, MessageBoxW, TITLE_W, MB_OK | MB_ICONERROR, exit(1))

// Warn
DEF_MSG_FN(msg_warn_a, char, _vsnprintf, MessageBoxA, TITLE_A, MB_OK | MB_ICONEXCLAMATION,)
DEF_MSG_FN(msg_warn_w, wchar_t, _vsnwprintf, MessageBoxW, TITLE_W, MB_OK | MB_ICONEXCLAMATION,)

// Info
DEF_MSG_FN(msg_info_a, char, _vsnprintf, MessageBoxA, TITLE_A, MB_OK | MB_ICONINFORMATION,)
DEF_MSG_FN(msg_info_w, wchar_t, _vsnwprintf, MessageBoxW, TITLE_W, MB_OK | MB_ICONINFORMATION,)
