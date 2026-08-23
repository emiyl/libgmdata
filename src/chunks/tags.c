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

    t->count = chunk.length == 0U ? 0U : 1U;
    return 0;
}

int TAGS_free(TagsChunk *t) {
    if (t == NULL) {
        return -1;
    }
    t->count = 0;
    return 0;
}
