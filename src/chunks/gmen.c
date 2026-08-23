#include "common.h"

int GMEN_free(GmenChunk *g);

int GMEN_parse(DataWin *dw) {
    Chunk chunk = {0};
    GmenChunk *g = &dw->gmen;

    memset(g, 0, sizeof(*g));

    if (get_chunk(dw, "GMEN", &chunk) != 0) {
        g->count = 0;
        g->code_ids = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "GMEN");

    read(&g->count, UInt32);

    g->code_ids = NULL;
    if (g->count == 0U) {
        return 0;
    }

    g->code_ids = (uint32_t *)safeCalloc(g->count, sizeof(uint32_t));
    if (g->code_ids == NULL) {
        g->count = 0;
        return -1;
    }

    for (uint32_t i = 0; i < g->count; ++i) {
        read(&g->code_ids[i], UInt32);
    }

    return 0;
}

int GMEN_free(GmenChunk *g) {
    if (g == NULL) {
        return -1;
    }
    if (g->code_ids != NULL) {
        free(g->code_ids);
        g->code_ids = NULL;
    }
    g->count = 0;
    return 0;
}
