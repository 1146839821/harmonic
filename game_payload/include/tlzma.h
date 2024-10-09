#pragma once

#include <stddef.h>

#pragma pack(push, 1)
struct tlzma_header {
    uint8_t  props[5];
    uint8_t  _1[3];
    uint32_t uncompressed_size;
    uint32_t compressed_size;
};
#pragma pack(pop)

int tlzma_test_header(void *header);

void *tlzma_decode(void *output, void *input, size_t *outUSize, size_t *outCSize);
