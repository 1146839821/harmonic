#pragma once

// Modified from https://stackoverflow.com/a/27950866

#include <stddef.h>
#include <stdint.h>

/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define __POLY 0x82f63b78

static inline uint32_t crc32c(uint32_t crc, const void *buf, size_t len) {
    const unsigned char *cbuf = (const unsigned char*)buf;

    crc = ~crc;

    while (len--) {
        crc ^= *cbuf++;
        for (int k = 0; k < 8; k++) {
            crc = crc & 1 ? (crc >> 1) ^ __POLY : crc >> 1;
        }
    }

    return ~crc;
}

#undef __POLY
