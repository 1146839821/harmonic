#pragma once

#include <windows.h>

#define DUMP_VERSION "2"

char* core_do_dump(size_t *outSize);
void core_do_load(char *dump, size_t size, void **outOEP);

void core_ace_init(void *a, void *b, void *c);
