#include "common.h"

int PSEM_parse(DataWin *dw) {
    Chunk chunk = {0};
    PsemChunk *p = &dw->psem;

    if (get_chunk(dw, "PSEM", &chunk) != 0) {
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
    Reader_init(reader, base, chunk.length, chunk.offset, "PSEM");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[PSEM_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[PSEM_parse] Unexpected version %u\n", version);
    }

    p->count = 0;
    return 0;
}

int PSEM_free(PsemChunk *p) {
    if (p == NULL) {
        return -1;
    }
    p->count = 0;
    return 0;
}
