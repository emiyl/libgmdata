#include "datawin.h"

#include "chunk_table.h"
#include "strings.h"
#include "log.h"

#include "chunks/chunks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void DataWin_reset(DataWin *dw) {
    if (dw == NULL) {
        return;
    }

    string_table_free(&dw->strings);
    chunk_table_free(&dw->chunks);
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
}

int DataWin_loadFile(DataWin *dw, const char *path) {
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

    DataWin_reset(dw);
    dw->file_data = buffer;
    dw->file_size = (size_t)size;
    dw->initialized = true;
    return 0;
}

int DataWin_parse(DataWin *dw) {
    if (dw == NULL || dw->file_data == NULL || !dw->initialized) {
        return -1;
    }

    if (parse_form_chunks(dw) != 0) {
        return -1;
    }

    Chunk strg = {0};
    assert(find_chunk(dw, "STRG", &strg) == 0);
    assert(parse_string_table(dw, strg.offset, strg.length) == 0);

    assert(GEN8_parse(dw) == 0);
    DataWin_bumpVersionTo(dw, dw->gen8.major, dw->gen8.minor, dw->gen8.release, dw->gen8.build);
    logInfo("Detected data.win version: %u.%u.%u.%u\n", dw->gen8.major, dw->gen8.minor, dw->gen8.release, dw->gen8.build);
    assert(OPTN_parse(dw) == 0);
    logInfo("Parsed chunk OPTN\n");
    assert(LANG_parse(dw) == 0);
    logInfo("Parsed chunk LANG\n");
    assert(EXTN_parse(dw) == 0);
    logInfo("Parsed chunk EXTN\n");
    assert(SOND_parse(dw) == 0);
    logInfo("Parsed chunk SOND\n");
    assert(AGRP_parse(dw) == 0);
    logInfo("Parsed chunk AGRP\n");
    assert(SPRT_parse(dw) == 0);
    logInfo("Parsed chunk SPRT\n");
    assert(BGND_parse(dw) == 0);
    logInfo("Parsed chunk BGND\n");
    assert(PATH_parse(dw) == 0);
    logInfo("Parsed chunk PATH\n");
    assert(SCPT_parse(dw) == 0);
    logInfo("Parsed chunk SCPT\n");
    assert(GLOB_parse(dw) == 0);
    logInfo("Parsed chunk GLOB\n");
    assert(SHDR_parse(dw) == 0);
    logInfo("Parsed chunk SHDR\n");
    assert(FONT_parse(dw) == 0);
    logInfo("Parsed chunk FONT\n");
    assert(TMLN_parse(dw) == 0);
    logInfo("Parsed chunk TMLN\n");

    return 0;
}

void DataWin_free(DataWin *dw) {
    if (dw == NULL) {
        return;
    }

    string_table_free(&dw->strings);
    chunk_table_free(&dw->chunks);

    GEN8_free(&dw->gen8);
    OPTN_free(&dw->optn);
    LANG_free(&dw->lang);
    EXTN_free(&dw->extn);
    SOND_free(&dw->sond);
    AGRP_free(&dw->agrp);
    SPRT_free(&dw->sprt);
    BGND_free(&dw->bgnd);
    PATH_free(&dw->path);
    SCPT_free(&dw->scpt);
    GLOB_free(&dw->glob);
    SHDR_free(&dw->shdr);
    FONT_free(&dw->font);
    TMLN_free(&dw->tmln);

    free(dw->file_data);
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
}

bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build) {
    const DetectedFormat* f = &dw->detected_format;
    if (f->major != major) return f->major > major;
    if (f->minor != minor) return f->minor > minor;
    if (f->release != release) return f->release > release;
    return f->build >= build;
}

void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build) {
    if (DataWin_isVersionAtLeast(dw, major, minor, release, build)) return;
    dw->detected_format.major = major;
    dw->detected_format.minor = minor;
    dw->detected_format.release = release;
    dw->detected_format.build = build;
}
