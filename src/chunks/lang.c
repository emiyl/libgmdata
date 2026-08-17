#include "lang.h"
#include "common.h"

int LANG_Parse(DataWin *dw) {
    Chunk chunk = {0};
    Lang *l = &dw->lang;

    if (find_chunk(dw, "LANG", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    reader_init(&reader, base, chunk.length);

    return 0;
}