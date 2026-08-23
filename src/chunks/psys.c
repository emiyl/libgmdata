#include "common.h"

int PSYS_parse(DataWin *dw) {
    Chunk chunk = {0};
    PsysChunk *p = &dw->psys;

    if (get_chunk(dw, "PSYS", &chunk) != 0) {
        p->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    if (chunk.length == 0U) {
        p->count = 0;
        return 0;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "PSYS");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[PSYS_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[PSYS_parse] Unexpected version %u\n", version);
    }

    p->count = 0;
    return 0;
}

int PSYS_free(PsysChunk *p) {
    if (p == NULL) {
        return -1;
    }
    p->count = 0;
    return 0;
}
