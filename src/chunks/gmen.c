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

    g->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int GMEN_free(GmenChunk *g) {
    if (g == NULL) {
        return -1;
    }
    g->count = 0;
    return 0;
}
