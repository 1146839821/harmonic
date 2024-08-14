#pragma once

#include <windows.h>
#include <winternl.h>

// https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrdllnotification
typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
    ULONG Flags;                    //Reserved.
    PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA {
    ULONG Flags;                    //Reserved.
    PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
    PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA {
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef void (*LdrDllNotification_t)(ULONG reason, const PLDR_DLL_NOTIFICATION_DATA data, void *context);

#define DYNAMIC_FN_TYPE(ret, name, args) typedef ret (*name##_t)args

#ifdef NTDLL_DYNAMIC_LINK_IMPL
    #define DYNAMIC_FN_VAR(name) extern name##_t name; name##_t name
#else
    #define DYNAMIC_FN_VAR(name) extern name##_t name
#endif

#define DYNAMIC_FN_DEF(ret, name, args) DYNAMIC_FN_TYPE(ret, name, args); DYNAMIC_FN_VAR(name)

DYNAMIC_FN_DEF(NTSTATUS, LdrRegisterDllNotification, (ULONG flags, LdrDllNotification_t notification, void *context, void **cookie));
DYNAMIC_FN_DEF(NTSTATUS, LdrUnregisterDllNotification, (void *cookie));

#ifdef NTDLL_DYNAMIC_LINK_IMPL
    #define DYNAMIC_FN_LINK(module, name) name = (name##_t)GetProcAddress(module, #name)

    static void ntdll_link() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");

        DYNAMIC_FN_LINK(ntdll, LdrRegisterDllNotification);
        DYNAMIC_FN_LINK(ntdll, LdrUnregisterDllNotification);
    }

    #undef DYNAMIC_FN_LINK
#endif

#undef DYNAMIC_FN_TYPE
#undef DYNAMIC_FN_VAR
#undef DYNAMIC_FN_DEF
