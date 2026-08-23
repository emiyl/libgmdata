#include "common.h"

int TAGS_parse(DataWin *dw) {
    Chunk chunk = {0};
    TagsChunk *t = &dw->tags;

    if (get_chunk(dw, "TAGS", &chunk) != 0) {
        t->count = 0;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    if (chunk.length == 0U) {
        t->count = 0;
        return 0;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "TAGS");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[TAGS_parse] Non-zero padding byte while aligning version\n");
        }
    }

    uint32_t version = 0;
    if (Reader_readUInt32(reader, &version) != 0 || version != 1U) {
        logWarn("[TAGS_parse] Unexpected version %u\n", version);
    }

    t->count = 0;
    return 0;
}

int TAGS_free(TagsChunk *t) {
    if (t == NULL) {
        return -1;
    }
    t->count = 0;
    return 0;
}
