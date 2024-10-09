#pragma once

#include <stddef.h>

void *tlzma_decode(void *output, void *input, size_t *outUSize, size_t *outCSize);
