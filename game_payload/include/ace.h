#pragma once

#include <windows.h>

HMODULE ace_load_shell_module();
HMODULE ace_load_base_module(void *init3Addr);
HMODULE ace_load_driver_module(void *coAddr);
