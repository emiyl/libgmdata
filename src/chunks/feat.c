#include "common.h"

int FEAT_parse(DataWin *dw) {
    Chunk chunk = {0};
    FeatChunk *f = &dw->feat;

    if (get_chunk(dw, "FEAT", &chunk) != 0) {
        f->count = 0;
        f->strings = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "FEAT");

    while (reader->cursor % 4U != 0U) {
        uint8_t pad = 0;
        read(&pad, UInt8);
        if (pad != 0U) {
            logWarn("[FEAT_parse] Non-zero padding byte found during version alignment\n");
        }
    }

    uint32_t count = 0;
    read(&count, UInt32);
    f->count = count;
    f->strings = NULL;

    if (count == 0U) {
        return 0;
    }

    f->strings = (const char **)safeMalloc(count * sizeof(const char *));
    if (f->strings == NULL) {
        f->count = 0;
        return -1;
    }

    for (uint32_t i = 0; i < count; ++i) {
        readString(&f->strings[i], dw);
    }

    return 0;
}

int FEAT_free(FeatChunk *f) {
    if (f == NULL) {
        return -1;
    }

    if (f->strings != NULL) {
        for (uint32_t i = 0; i < f->count; ++i) {
            free((void *)f->strings[i]);
            f->strings[i] = NULL;
        }
        free(f->strings);
        f->strings = NULL;
    }

    f->count = 0;
    return 0;
}
