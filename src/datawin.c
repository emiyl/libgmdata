#include "datawin.h"

#include "chunks.h"
#include "chunks/optn.h"
#include "strings.h"

#include "chunks/gen8.h"
#include "chunks/optn.h"
#include "chunks/lang.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void reset(DataWin *dw) {
    if (dw == NULL) {
        return;
    }

    string_table_free(&dw->strings);
    chunk_table_free(&dw->chunks);
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
}

int load_file(DataWin *dw, const char *path) {
    if (dw == NULL || path == NULL) {
        return -1;
    }

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    uint8_t *buffer = (uint8_t *)malloc((size_t)size);
    if (buffer == NULL) {
        fclose(fp);
        return -1;
    }

    if (fread(buffer, 1, (size_t)size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    reset(dw);
    dw->file_data = buffer;
    dw->file_size = (size_t)size;
    dw->initialized = true;
    return 0;
}

int parse(DataWin *dw) {
    if (dw == NULL || dw->file_data == NULL || !dw->initialized) {
        return -1;
    }

    if (parse_form_chunks(dw) != 0) {
        return -1;
    }

    Chunk strg = {0};
    assert(find_chunk(dw, "STRG", &strg) == 0);
    assert(parse_string_table(dw, strg.offset, strg.length) == 0);

    assert(GEN8_Parse(dw) == 0);
    assert(OPTN_Parse(dw) == 0);
    assert(LANG_Parse(dw) == 0);

    return 0;
}

void datawin_free(DataWin *dw) {
    if (dw == NULL) {
        return;
    }

    string_table_free(&dw->strings);
    chunk_table_free(&dw->chunks);
    free(dw->file_data);
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
}
