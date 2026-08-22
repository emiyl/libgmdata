#include "common.h"
#include "../strings.h"

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
        if (ptrs[i] == 0) continue;
        s->strings[i] = resolve_string_ptr(dw, base, ptrs[i]);
    }

    free(ptrs);
    return 0;
}

int STRG_free(StrgChunk *s) {
    if (s == NULL) return -1;
    free(s->strings);
    s->strings = NULL;
    s->count = 0;
    return 0;
}
