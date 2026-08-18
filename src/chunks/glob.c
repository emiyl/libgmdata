#include "common.h"

int GLOB_parse(DataWin *dw) {
    Chunk chunk = {0};
    Glob *g = &dw->glob;

    if (find_chunk(dw, "GLOB", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "GLOB");

    Reader_readUInt32(&reader, &g->count);
    if (g->count == 0) {
        g->codeIds = NULL;
        return 0;
    }

    g->codeIds = (int32_t*)safeMalloc(g->count * sizeof(int32_t));
    repeat(g->count, i) {
        Reader_readInt32(&reader, &g->codeIds[i]);
    }

    return 0;
}

int GLOB_free(Glob *g) {
    if (g == NULL) return -1;
    free(g->codeIds);
    g->codeIds = NULL;
    g->count = 0;
    return 0;
}