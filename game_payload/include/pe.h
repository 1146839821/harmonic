#pragma once

#include <windows.h>

IMAGE_SECTION_HEADER *pe_find_section(const void *module, const char *section);

void *pe_find_entry_point(HMODULE module);
