#include "common.h"
#include "../strings.h"

static const char *STRG_dup_string(const DataWin *dw, uint32_t absolute_offset) {
    if (dw == NULL || dw->file_data == NULL) {
        return NULL;
    }

    if (absolute_offset + sizeof(uint32_t) > dw->file_size) {
        return NULL;
    }

    uint32_t length = read_u32_le_at(dw->file_data, dw->file_size, absolute_offset);
    if (absolute_offset + sizeof(uint32_t) + length > dw->file_size) {
        return NULL;
    }

    char *copy = (char *)malloc((size_t)length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, dw->file_data + absolute_offset + sizeof(uint32_t), length);
    copy[length] = '\0';
    return copy;
}

static int STRG_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    const char **str = (const char **)out;
    if (Reader_readUInt32(reader, &(uint32_t){0}) != 0) {
        return -1;
    }
    *str = NULL;
    return 0;
}

static int STRG_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    const char **str = (const char **)out;
    *str = NULL;
    return 0;
}

int STRG_parse(DataWin *dw) {
    Chunk chunk = {0};
    StrgChunk *s = &dw->strg;

    if (get_chunk(dw, "STRG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "STRG");

    uint32_t count = 0;
    uint32_t *ptrs = NULL;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;

    s->count = count;
    s->strings = NULL;
    if (count == 0) {
        free(ptrs);
        return 0;
    }

    s->strings = (const char **)safeCalloc(count, sizeof(const char *));
    repeat(count, i) {
        if (ptrs[i] == 0) {
            s->strings[i] = NULL;
            continue;
        }

        uint32_t absolute_offset = chunk.offset + ptrs[i];
        s->strings[i] = STRG_dup_string(dw, absolute_offset);
    }

    free(ptrs);
    return 0;
}

int STRG_free(StrgChunk *s) {
    if (s == NULL) return -1;
    if (s->strings != NULL) {
        for (uint32_t i = 0; i < s->count; ++i) {
            free((void *)s->strings[i]);
        }
    }
    free(s->strings);
    s->strings = NULL;
    s->count = 0;
    return 0;
}
