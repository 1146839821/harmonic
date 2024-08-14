#pragma once

#include <windows.h>

#define LAUNCHER_INJECT_DLL  L"launcher_payload.dll"
#define GAME_INJECT_DLL      L"game_payload.dll"

void inject(HANDLE process, const void *payload, size_t payloadSize, const wchar_t *dllPath, char breakImports);
