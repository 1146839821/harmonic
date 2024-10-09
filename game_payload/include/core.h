#pragma once

#include <windows.h>

#define DUMP_VERSION "3"

char* core_do_dump(size_t *outSize);
void core_do_load(char *dump, size_t size, void **outOEP);

int core_ace_init(void *a, void *b, void *c);
void *core_drv_co(void *a);
