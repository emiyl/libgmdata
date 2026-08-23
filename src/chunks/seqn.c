#include "common.h"

int SEQN_parse(DataWin *dw) {
    Chunk chunk = {0};
    SeqnChunk *s = &dw->seqn;

    if (get_chunk(dw, "SEQN", &chunk) != 0) {
        s->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    if (chunk.length == 0U) {
        s->count = 0;
        return 0;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "SEQN");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[SEQN_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[SEQN_parse] Unexpected version %u\n", version);
    }

    s->count = 0;
    return 0;
}

int SEQN_free(SeqnChunk *s) {
    if (s == NULL) {
        return -1;
    }
    s->count = 0;
    return 0;
}
