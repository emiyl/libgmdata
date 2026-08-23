#include "common.h"

int UILR_parse(DataWin *dw) {
    Chunk chunk = {0};
    UilrChunk *u = &dw->uilr;

    if (get_chunk(dw, "UILR", &chunk) != 0) {
        u->count = 0;
        return 0;
    }

    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    u->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int UILR_free(UilrChunk *u) {
    if (u == NULL) {
        return -1;
    }
    u->count = 0;
    return 0;
}
