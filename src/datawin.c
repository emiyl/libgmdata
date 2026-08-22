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
    memset(&dw->gen8, 0, sizeof(dw->gen8));
    memset(&dw->optn, 0, sizeof(dw->optn));
    memset(&dw->lang, 0, sizeof(dw->lang));
    memset(&dw->extn, 0, sizeof(dw->extn));
    memset(&dw->sond, 0, sizeof(dw->sond));
    memset(&dw->agrp, 0, sizeof(dw->agrp));
    memset(&dw->sprt, 0, sizeof(dw->sprt));
    memset(&dw->bgnd, 0, sizeof(dw->bgnd));
    memset(&dw->tpag, 0, sizeof(dw->tpag));
    memset(&dw->path, 0, sizeof(dw->path));
    memset(&dw->scpt, 0, sizeof(dw->scpt));
    memset(&dw->glob, 0, sizeof(dw->glob));
    memset(&dw->code, 0, sizeof(dw->code));
    memset(&dw->vari, 0, sizeof(dw->vari));
    memset(&dw->func, 0, sizeof(dw->func));
    memset(&dw->shdr, 0, sizeof(dw->shdr));
    memset(&dw->font, 0, sizeof(dw->font));
    memset(&dw->tmln, 0, sizeof(dw->tmln));
    memset(&dw->objt, 0, sizeof(dw->objt));
    memset(&dw->room, 0, sizeof(dw->room));
    memset(&dw->acrv, 0, sizeof(dw->acrv));
    memset(&dw->strg, 0, sizeof(dw->strg));
    memset(&dw->txtr, 0, sizeof(dw->txtr));
    memset(&dw->audo, 0, sizeof(dw->audo));
    memset(&dw->detectedFormat, 0, sizeof(dw->detectedFormat));
    dw->mappedFile = NULL;
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
}

int DataWin_loadFile(DataWin *dw, const char *path) {
    if (dw == NULL || path == NULL) {
        return -1;
    }

    if (dw->initialized || dw->file_data != NULL || dw->chunks.count != 0 || dw->strings.count != 0) {
        DataWin_free(dw);
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
    if (get_chunk(dw, "STRG", &strg) >= 0) {
        assert(parse_string_table(dw, strg.offset, strg.length) == 0);
    }
    
    // bool codeExists = false;
    // if (chunk_exists(dw, "CODE") == 0) {
    //     codeExists = true;
    // }

    if (chunk_exists(dw, "ACRV") == 0 || chunk_exists(dw, "SEQN") == 0 || chunk_exists(dw, "TAGS") == 0) {
        DataWin_bumpVersionTo(dw, 2, 3, 0, 0);
    } else if (chunk_exists(dw, "FEDS") == 0) {
        DataWin_bumpVersionTo(dw, 2, 3, 6, 0);
    } else if (chunk_exists(dw, "FEAT") == 0) {
        DataWin_bumpVersionTo(dw, 2022, 8, 0, 0);
    } else if (chunk_exists(dw, "UILR") == 0) {
        DataWin_bumpVersionTo(dw, 2024, 13, 0, 0);
    } else if (chunk_exists(dw, "PSEM") == 0 || chunk_exists(dw, "PSYS") == 0) {
        DataWin_bumpVersionTo(dw, 2023, 2, 0, 0);
    }

    assert(GEN8_parse(dw) == 0);
    DataWin_bumpVersionTo(dw, dw->gen8.major, dw->gen8.minor, dw->gen8.release, dw->gen8.build);
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
    assert(FONT_parse(dw) == 0);
    logInfo("Parsed chunk FONT\n");
    assert(TPAG_parse(dw) == 0);
    logInfo("Parsed chunk TPAG\n");
    assert(PATH_parse(dw) == 0);
    logInfo("Parsed chunk PATH\n");
    assert(SCPT_parse(dw) == 0);
    logInfo("Parsed chunk SCPT\n");
    assert(GLOB_parse(dw) == 0);
    logInfo("Parsed chunk GLOB\n");
    assert(CODE_parse(dw) == 0);
    logInfo("Parsed chunk CODE\n");
    assert(VARI_parse(dw) == 0);
    logInfo("Parsed chunk VARI\n");
    assert(FUNC_parse(dw) == 0);
    logInfo("Parsed chunk FUNC\n");
    assert(SHDR_parse(dw) == 0);
    logInfo("Parsed chunk SHDR\n");
    assert(TMLN_parse(dw) == 0);
    logInfo("Parsed chunk TMLN\n");
    assert(OBJT_parse(dw) == 0);
    logInfo("Parsed chunk OBJT\n");
    assert(ROOM_parse(dw) == 0);
    logInfo("Parsed chunk ROOM\n");
    assert(ACRV_parse(dw) == 0);
    logInfo("Parsed chunk ACRV\n");
    assert(TXTR_parse(dw) == 0);
    logInfo("Parsed chunk TXTR\n");
    assert(AUDO_parse(dw) == 0);
    logInfo("Parsed chunk AUDO\n");

    return 0;
}

int DataWin_free(DataWin *dw) {
    if (dw == NULL) {
        return -1;
    }

    int result = 0;

    if(string_table_free(&dw->strings)) {
        logWarn("[DataWin_free] Failed to free string table\n");
        result = -1;
    };
    if(chunk_table_free(&dw->chunks)) {
        logWarn("[DataWin_free] Failed to free chunk table\n");
        result = -1;
    };

    if (GEN8_free(&dw->gen8)) {
        logWarn("[DataWin_free] Failed to free GEN8 chunk\n");
        result = -1;
    }
    if (OPTN_free(&dw->optn)) {
        logWarn("[DataWin_free] Failed to free OPTN chunk\n");
        result = -1;
    };
    if (LANG_free(&dw->lang)) {
        logWarn("[DataWin_free] Failed to free LANG chunk\n");
        result = -1;
    }
    if (EXTN_free(&dw->extn)) {
        logWarn("[DataWin_free] Failed to free EXTN chunk\n");
        result = -1;
    }
    if (SOND_free(&dw->sond)) {
        logWarn("[DataWin_free] Failed to free SOND chunk\n");
        result = -1;
    }
    if (AGRP_free(&dw->agrp)) {
        logWarn("[DataWin_free] Failed to free AGRP chunk\n");
        result = -1;
    }
    if (SPRT_free(&dw->sprt)) {
        logWarn("[DataWin_free] Failed to free SPRT chunk\n");
        result = -1;
    }
    if (BGND_free(&dw->bgnd)) {
        logWarn("[DataWin_free] Failed to free BGND chunk\n");
        result = -1;
    }
    if (TPAG_free(&dw->tpag)) {
        logWarn("[DataWin_free] Failed to free TPAG chunk\n");
        result = -1;
    }
    if (PATH_free(&dw->path)) {
        logWarn("[DataWin_free] Failed to free PATH chunk\n");
        result = -1;
    }
    if (SCPT_free(&dw->scpt)) {
        logWarn("[DataWin_free] Failed to free SCPT chunk\n");
        result = -1;
    }
    if (GLOB_free(&dw->glob)) {
        logWarn("[DataWin_free] Failed to free GLOB chunk\n");
        result = -1;
    }
    if (CODE_free(&dw->code)) {
        logWarn("[DataWin_free] Failed to free CODE chunk\n");
        result = -1;
    }
    if (VARI_free(&dw->vari)) {
        logWarn("[DataWin_free] Failed to free VARI chunk\n");
        result = -1;
    }
    if (FUNC_free(&dw->func)) {
        logWarn("[DataWin_free] Failed to free FUNC chunk\n");
        result = -1;
    }
    if (SHDR_free(&dw->shdr)) {
        logWarn("[DataWin_free] Failed to free SHDR chunk\n");
        result = -1;
    }
    if (FONT_free(&dw->font)) {
        logWarn("[DataWin_free] Failed to free FONT chunk\n");
        result = -1;
    }
    if (TMLN_free(&dw->tmln)) {
        logWarn("[DataWin_free] Failed to free TMLN chunk\n");
        result = -1;
    }
    if (OBJT_free(&dw->objt)) {
        logWarn("[DataWin_free] Failed to free OBJT chunk\n");
        result = -1;
    }
    if (ROOM_free(&dw->room)) {
        logWarn("[DataWin_free] Failed to free ROOM chunk\n");
        result = -1;
    }
    if (ACRV_free(&dw->acrv)) {
        logWarn("[DataWin_free] Failed to free ACRV chunk\n");
        result = -1;
    }
    if (TXTR_free(&dw->txtr)) {
        logWarn("[DataWin_free] Failed to free TXTR chunk\n");
        result = -1;
    }
    if (AUDO_free(&dw->audo)) {
        logWarn("[DataWin_free] Failed to free AUDO chunk\n");
        result = -1;
    }

    free(dw->file_data);
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->initialized = false;
    return result;
}

bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build) {
    const DetectedFormat* f = &dw->detectedFormat;
    if (f->major != major) return f->major > major;
    if (f->minor != minor) return f->minor > minor;
    if (f->release != release) return f->release > release;
    return f->build >= build;
}

void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build) {
    if (DataWin_isVersionAtLeast(dw, major, minor, release, build)) return;
    dw->detectedFormat.major = major;
    dw->detectedFormat.minor = minor;
    dw->detectedFormat.release = release;
    dw->detectedFormat.build = build;
}
