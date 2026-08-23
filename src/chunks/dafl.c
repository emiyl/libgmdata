#include "common.h"

int DAFL_parse(DataWin *dw) {
    Chunk chunk = {0};
    DaflChunk *d = &dw->dafl;

    if (get_chunk(dw, "DAFL", &chunk) != 0) {
        d->count = 0;
        return 0;
    }

    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    d->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int DAFL_free(DaflChunk *d) {
    if (d == NULL) {
        return -1;
    }
    d->count = 0;
    return 0;
}
