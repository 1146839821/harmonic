#include <pe.h>

IMAGE_SECTION_HEADER *pe_find_section(const void *module, const char *section) {
    const char *cModule = (const char*)module;

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)module;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(cModule + dosHeader->e_lfanew);

    WORD sectionCount = ntHeaders->FileHeader.NumberOfSections;
    IMAGE_SECTION_HEADER* sectionHeader = (IMAGE_SECTION_HEADER*)(ntHeaders + 1);

    for (WORD i = 0; i < sectionCount; i++) {
        if (strncmp((char*)sectionHeader->Name, section, 8) == 0) {
            return sectionHeader;
        }

        sectionHeader++;
    }

    return NULL;
}

void *pe_find_entry_point(HMODULE module) {
    char *cModule = (char*)module;

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)module;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*)(cModule + dosHeader->e_lfanew);

    return cModule + ntHeaders->OptionalHeader.AddressOfEntryPoint;
}
