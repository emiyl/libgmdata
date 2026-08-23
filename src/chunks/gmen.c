#include "common.h"

int GMEN_parse(DataWin *dw) {
    Chunk chunk = {0};
    GmenChunk *g = &dw->gmen;

    if (get_chunk(dw, "GMEN", &chunk) != 0) {
        g->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    if (chunk.length == 0U) {
        g->count = 0;
        return 0;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "GMEN");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[GMEN_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[GMEN_parse] Unexpected version %u\n", version);
    }

    g->count = 0;
    return 0;
}

int GMEN_free(GmenChunk *g) {
    if (g == NULL) {
        return -1;
    }
    g->count = 0;
    return 0;
}
