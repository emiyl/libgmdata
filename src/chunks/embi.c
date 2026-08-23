#include "common.h"

int EMBI_parse(DataWin *dw) {
    Chunk chunk = {0};
    EmbiChunk *e = &dw->embi;

    if (get_chunk(dw, "EMBI", &chunk) != 0) {
        e->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    if (chunk.length == 0U) {
        e->count = 0;
        return 0;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "EMBI");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[EMBI_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[EMBI_parse] Unexpected version %u\n", version);
    }

    e->count = 0;
    return 0;
}

int EMBI_free(EmbiChunk *e) {
    if (e == NULL) {
        return -1;
    }
    e->count = 0;
    return 0;
}
