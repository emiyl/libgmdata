#include "gmdata.h"

#include "chunk_table.h"
#include "strings.h"
#include "log.h"

#include "chunks/chunks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

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
    memset(&dw->feds, 0, sizeof(dw->feds));
    memset(&dw->feat, 0, sizeof(dw->feat));
    memset(&dw->seqn, 0, sizeof(dw->seqn));
    memset(&dw->tags, 0, sizeof(dw->tags));
    memset(&dw->embi, 0, sizeof(dw->embi));
    memset(&dw->psem, 0, sizeof(dw->psem));
    memset(&dw->psys, 0, sizeof(dw->psys));
    memset(&dw->gmen, 0, sizeof(dw->gmen));
    memset(&dw->dafl, 0, sizeof(dw->dafl));
    memset(&dw->uilr, 0, sizeof(dw->uilr));
    memset(&dw->stat, 0, sizeof(dw->stat));
    memset(&dw->tgin, 0, sizeof(dw->tgin));
    memset(&dw->strg, 0, sizeof(dw->strg));
    memset(&dw->txtr, 0, sizeof(dw->txtr));
    memset(&dw->audo, 0, sizeof(dw->audo));
    memset(&dw->detectedFormat, 0, sizeof(dw->detectedFormat));
    dw->mappedFile = NULL;
    dw->file_data = NULL;
    dw->file_size = 0;
    dw->lazyLoadRooms = false;
    dw->lazyLoadTextures = false;
    dw->lazyLoadAudio = false;
    dw->initialized = false;
}

void DataWin_initParserOptions(DataWinParserOptions *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->parseGen8 = true;
    options->parseOptn = true;
    options->parseLang = true;
    options->parseExtn = true;
    options->parseSond = true;
    options->parseAgrp = true;
    options->parseSprt = true;
    options->parseBgnd = true;
    options->parsePath = true;
    options->parseScpt = true;
    options->parseGlob = true;
    options->parseShdr = true;
    options->parseFont = true;
    options->parseTmln = true;
    options->parseObjt = true;
    options->parseRoom = true;
    options->parseTpag = true;
    options->parseCode = true;
    options->parseVari = true;
    options->parseFunc = true;
    options->parseStrg = true;
    options->parseTgin = true;
    options->parseTxtr = true;
    options->parseAudo = true;
    options->parseAcrv = true;
    options->parseFeds = true;
    options->parseFeat = true;
    options->parseSeqn = true;
    options->parseTags = true;
    options->parseEmbi = true;
    options->parsePsem = true;
    options->parsePsys = true;
    options->parseGmen = true;
    options->parseDafl = true;
    options->parseUilr = true;
    options->parseStat = true;
    options->loadType = DATAWINLOADTYPE_LOAD_PER_CHUNK;
}

void DataWin_applyParserOptions(DataWin *dw, const DataWinParserOptions *options) {
    if (dw == NULL) {
        return;
    }

    DataWinParserOptions effective = {0};
    DataWin_initParserOptions(&effective);

    if (options != NULL) {
        effective = *options;
    }

    dw->lazyLoadRooms = effective.lazyLoadRooms;
    dw->lazyLoadTextures = effective.lazyLoadTextures;
    dw->lazyLoadAudio = effective.lazyLoadAudio;

    if (effective.loadType == DATAWINLOADTYPE_MAP_FILE) {
        logInfo("[DataWin_applyParserOptions] MAP_FILE load mode selected\n");
    }
}

static void DataWin_emitProgress(
    DataWin *dw,
    const DataWinParserOptions *options,
    const char *chunkName,
    int chunkIndex,
    int totalChunks
) {
    if (dw == NULL || options == NULL || options->progressCallback == NULL) {
        return;
    }

    options->progressCallback(chunkName, chunkIndex, totalChunks, dw, options->progressCallbackUserData);
}

typedef struct {
    const char *name;
    int (*parser)(DataWin *dw);
    size_t field_offset;
    size_t field_size;
    DataWin value;
    int status;
} DataWinChunkParseTask;

static void *DataWin_chunkParseTask_worker(void *arg) {
    DataWinChunkParseTask *task = (DataWinChunkParseTask *)arg;
    if (task == NULL) {
        return NULL;
    }

    task->status = task->parser(&task->value);
    return NULL;
}

static int DataWin_parseChunkTasksParallel(
    DataWin *dw,
    const DataWinChunkParseTask *tasks,
    size_t taskCount
) {
    if (dw == NULL || tasks == NULL || taskCount == 0U) {
        return 0;
    }

    if (taskCount == 1U) {
        return tasks[0].parser(dw);
    }

    pthread_t *threads = (pthread_t *)malloc(taskCount * sizeof(*threads));
    DataWinChunkParseTask *runtime = (DataWinChunkParseTask *)malloc(taskCount * sizeof(*runtime));
    if (threads == NULL || runtime == NULL) {
        free(threads);
        free(runtime);
        return -1;
    }

    for (size_t i = 0; i < taskCount; ++i) {
        runtime[i] = tasks[i];
        runtime[i].value = *dw;
        runtime[i].status = 0;
        if (pthread_create(&threads[i], NULL, DataWin_chunkParseTask_worker, &runtime[i]) != 0) {
            for (size_t j = 0; j < i; ++j) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(runtime);
            return -1;
        }
    }

    for (size_t i = 0; i < taskCount; ++i) {
        pthread_join(threads[i], NULL);
    }

    for (size_t i = 0; i < taskCount; ++i) {
        if (runtime[i].status != 0) {
            free(threads);
            free(runtime);
            return -1;
        }
        memcpy((char *)dw + runtime[i].field_offset,
               (char *)&runtime[i].value + runtime[i].field_offset,
               runtime[i].field_size);
    }

    free(threads);
    free(runtime);
    return 0;
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

int DataWin_parseWithOptions(DataWin *dw, const DataWinParserOptions *options) {
    if (dw == NULL || dw->file_data == NULL || !dw->initialized) {
        return -1;
    }

    DataWinParserOptions effective = {0};
    DataWin_initParserOptions(&effective);
    if (options != NULL) {
        effective = *options;
    }
    DataWin_applyParserOptions(dw, &effective);

    if (DataWin_detectVersionFromFile(dw->file_data, dw->file_size, &dw->detectedFormat) == 0) {
        logInfo("[DataWin_parseWithOptions] Detected format: %u.%u.%u.%u\n",
                dw->detectedFormat.major,
                dw->detectedFormat.minor,
                dw->detectedFormat.release,
                dw->detectedFormat.build);
    }

    if (parse_form_chunks(dw) != 0) {
        return -1;
    }

    if (effective.parseStrg) {
        Chunk strg = {0};
        if (get_chunk(dw, "STRG", &strg) >= 0) {
            assert(parse_string_table(dw, strg.offset, strg.length) == 0);
        }
    }

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

    int totalChunks = (int)dw->chunks.count;
    int parsedChunkCount = 0;

    if (effective.parseGen8) {
        assert(GEN8_parse(dw) == 0);
        DataWin_bumpVersionTo(dw, dw->gen8.major, dw->gen8.minor, dw->gen8.release, dw->gen8.build);
        DataWin_emitProgress(dw, &effective, "GEN8", parsedChunkCount, totalChunks);
        parsedChunkCount++;
    }

    #define DATAWIN_TASK(chunkName, chunkMember) \
        (DataWinChunkParseTask){ #chunkName, chunkName##_parse, offsetof(DataWin, chunkMember), sizeof(((DataWin *)0)->chunkMember), NULL, 0 }

    #define DATAWIN_COMMON_TASK_CAPACITY 48U
    DataWinChunkParseTask commonTasks[DATAWIN_COMMON_TASK_CAPACITY] = {{0}};
    size_t commonTaskCount = 0;

    #define add_common_task(option, chunkName, chunkMember) do { \
        if (effective.parse##option) { \
            assert(commonTaskCount < DATAWIN_COMMON_TASK_CAPACITY); \
            commonTasks[commonTaskCount++] = DATAWIN_TASK(chunkName, chunkMember); \
        } \
    } while (0)

    add_common_task(Optn, OPTN, optn);
    add_common_task(Lang, LANG, lang);
    add_common_task(Extn, EXTN, extn);
    add_common_task(Sond, SOND, sond);
    add_common_task(Agrp, AGRP, agrp);
    add_common_task(Sprt, SPRT, sprt);
    add_common_task(Bgnd, BGND, bgnd);
    add_common_task(Font, FONT, font);
    add_common_task(Tpag, TPAG, tpag);
    add_common_task(Path, PATH, path);
    add_common_task(Scpt, SCPT, scpt);
    add_common_task(Glob, GLOB, glob);
    add_common_task(Code, CODE, code);
    add_common_task(Vari, VARI, vari);
    add_common_task(Func, FUNC, func);
    add_common_task(Shdr, SHDR, shdr);
    add_common_task(Tmln, TMLN, tmln);
    add_common_task(Objt, OBJT, objt);
    add_common_task(Room, ROOM, room);
    add_common_task(Strg, STRG, strg);
    add_common_task(Tgin, TGIN, tgin);
    add_common_task(Txtr, TXTR, txtr);
    add_common_task(Audo, AUDO, audo);
    add_common_task(Acrv, ACRV, acrv);
    add_common_task(Feds, FEDS, feds);
    add_common_task(Feat, FEAT, feat);
    add_common_task(Seqn, SEQN, seqn);
    add_common_task(Tags, TAGS, tags);
    add_common_task(Embi, EMBI, embi);
    add_common_task(Gmen, GMEN, gmen);
    add_common_task(Dafl, DAFL, dafl);
    add_common_task(Uilr, UILR, uilr);
    add_common_task(Stat, STAT, stat);

    if (commonTaskCount > 0U) {
        if (DataWin_parseChunkTasksParallel(dw, commonTasks, commonTaskCount) != 0) {
            return -1;
        }

        for (size_t i = 0; i < commonTaskCount; ++i) {
            DataWin_emitProgress(dw, &effective, commonTasks[i].name, parsedChunkCount, totalChunks);
            parsedChunkCount++;
        }
    }

    if (effective.parsePsem) {
        DataWin_emitProgress(dw, &effective, "PSEM", parsedChunkCount, totalChunks);
        parsedChunkCount++;
        assert(PSEM_parse(dw) == 0);
    }

    if (effective.parsePsys) {
        DataWin_emitProgress(dw, &effective, "PSYS", parsedChunkCount, totalChunks);
        parsedChunkCount++;
        assert(PSYS_parse(dw) == 0);
    }

    if (effective.progressCallback != NULL && totalChunks > 0 && parsedChunkCount < totalChunks) {
        DataWin_emitProgress(dw, &effective, "DONE", totalChunks - 1, totalChunks);
    }

    return 0;
}

int DataWin_parse(DataWin *dw) {
    return DataWin_parseWithOptions(dw, NULL);
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
    if (FEDS_free(&dw->feds)) {
        logWarn("[DataWin_free] Failed to free FEDS chunk\n");
        result = -1;
    }
    if (FEAT_free(&dw->feat)) {
        logWarn("[DataWin_free] Failed to free FEAT chunk\n");
        result = -1;
    }
    if (SEQN_free(&dw->seqn)) {
        logWarn("[DataWin_free] Failed to free SEQN chunk\n");
        result = -1;
    }
    if (TAGS_free(&dw->tags)) {
        logWarn("[DataWin_free] Failed to free TAGS chunk\n");
        result = -1;
    }
    if (EMBI_free(&dw->embi)) {
        logWarn("[DataWin_free] Failed to free EMBI chunk\n");
        result = -1;
    }
    if (PSEM_free(&dw->psem)) {
        logWarn("[DataWin_free] Failed to free PSEM chunk\n");
        result = -1;
    }
    if (PSYS_free(&dw->psys)) {
        logWarn("[DataWin_free] Failed to free PSYS chunk\n");
        result = -1;
    }
    if (GMEN_free(&dw->gmen)) {
        logWarn("[DataWin_free] Failed to free GMEN chunk\n");
        result = -1;
    }
    if (DAFL_free(&dw->dafl)) {
        logWarn("[DataWin_free] Failed to free DAFL chunk\n");
        result = -1;
    }
    if (UILR_free(&dw->uilr)) {
        logWarn("[DataWin_free] Failed to free UILR chunk\n");
        result = -1;
    }
    if (STAT_free(&dw->stat)) {
        logWarn("[DataWin_free] Failed to free STAT chunk\n");
        result = -1;
    }
    if (TGIN_free(&dw->tgin)) {
        logWarn("[DataWin_free] Failed to free TGIN chunk\n");
        result = -1;
    }
    if (STRG_free(&dw->strg)) {
        logWarn("[DataWin_free] Failed to free STRG chunk\n");
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

    const bool current_is_unset =
        f->major == 0 && f->minor == 0 && f->release == 0 && f->build == 0;

    if (current_is_unset) {
        return major == 0 && minor == 0 && release == 0 && build == 0;
    }

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
