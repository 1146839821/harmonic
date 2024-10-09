#include <stdint.h>
#include <utils.h>
#include <msg.h>
#include <main.h>
#include <LzmaDec.h>

#include <tlzma.h>

#define PROPS_XK (0x9d^0xf3)
#define PROPS_AK (0x65^0xa2)
#define USIZE_XK (0x218d5a0a^0xab8832cb)
#define USIZE_AK (0x5e98c630^0x66e44294)
#define CSIZE_XK (0x6dfc36ff^0xfc4c53bf)
#define CSIZE_AK (0xc68dd929^0x78bdb6e9)

#define GOOD_PROPS (0xcd386d0f^0x996f3a58)

static void* lzma_alloc(ISzAllocPtr p, size_t size) {
    return malloc(size);
}

static void lzma_free(ISzAllocPtr p, void *address) {
    free(address);
}

static void parse_header(void *header, struct tlzma_header *out) {
    struct tlzma_header *raw = (struct tlzma_header *)header;

    for (size_t i = 0; i < UTILS_COUNT(raw->props); i++) {
        out->props[i] = (raw->props[i] ^ PROPS_XK) + PROPS_AK;
    }
    out->uncompressed_size = (raw->uncompressed_size ^ USIZE_XK) + USIZE_AK;
    out->compressed_size = (raw->compressed_size ^ CSIZE_XK) + CSIZE_AK;
}

int tlzma_test_header(void *header) {
    struct tlzma_header *raw = (struct tlzma_header *)header;
    return *((uint32_t*)(raw->props + 1)) == GOOD_PROPS;
}

void *tlzma_decode(void *output, void *input, size_t *outUSize, size_t *outCSize) {
    struct tlzma_header header;
    parse_header(input, &header);

    void *data = ((uint8_t*)input) + sizeof(struct tlzma_header);
    void *buf = output ? output : malloc(header.uncompressed_size);
    
    ISzAlloc alloc = { &lzma_alloc, &lzma_free };
    SizeT destLen = header.uncompressed_size;
    SizeT srcLen = header.compressed_size;
    ELzmaStatus status;
    SRes res = LzmaDecode(buf, &destLen, data, &srcLen, header.props, sizeof(header.props), LZMA_FINISH_END, &status, &alloc);

    if (res != SZ_OK) {
        msg_err_a("Failed to decode LZMA at %p!\nSRes: %u\nStatus: %u\n\n" ISSUE_SUFFIX, input, res, status);
    }

    if (outUSize) *outUSize = destLen;
    if (outCSize) *outCSize = srcLen + sizeof(struct tlzma_header);
    return buf;
}
