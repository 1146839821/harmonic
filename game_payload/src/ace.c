#include <msg.h>
#include <utils.h>

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
