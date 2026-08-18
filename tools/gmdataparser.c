#include "../include/datawin.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

void GEN8_Print(const Gen8 *g);
void OPTN_Print(const Optn *o);
void LANG_Print(const Lang *l);
void EXTN_Print(const Extn *e);
void SOND_Print(const Sond *s);
void AGRP_Print(const Agrp *a);
void SPRT_Print(const Sprt *s);
void BGND_Print(const Bgnd *b);
void PATH_Print(const Path *p);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s [options] <data.win>\n"
            "\n"
            "options:\n"
            "  --print <name>    Print a section\n"
            "                    gen8, optn, etc, or *\n",
            argv[0]);
        return 1;
    }

    const char *filename = NULL;
    bool printGen8 = false;
    bool printOptn = false;
    bool printLang = false;
    bool printExtn = false;
    bool printSond = false;
    bool printAgrp = false;
    bool printSprt = false;
    bool printBgnd = false;
    bool printPath = false;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--print") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--print requires an argument\n");
                return 1;
            }

            const char *name = argv[++i];

            if (strcmp(name, "*") == 0) {
                printGen8 = true;
                printOptn = true;
                printLang = true;
                printExtn = true;
                printSond = true;
                printAgrp = true;
                printSprt = true;
                printBgnd = true;
                printPath = true;
            } else if (strcmp(name, "gen8") == 0) {
                printGen8 = true;
            } else if (strcmp(name, "optn") == 0) {
                printOptn = true;
            } else if (strcmp(name, "lang") == 0) {
                printLang = true;
            } else if (strcmp(name, "extn") == 0) {
                printExtn = true;
            } else if (strcmp(name, "sond") == 0) {
                printSond = true;
            } else if (strcmp(name, "agrp") == 0) {
                printAgrp = true;
            } else if (strcmp(name, "sprt") == 0) {
                printSprt = true;
            } else if (strcmp(name, "bgnd") == 0) {
                printBgnd = true;
            } else if (strcmp(name, "path") == 0) {
                printPath = true;
            } else {
                fprintf(stderr, "unknown print target: %s\n", name);
                return 1;
            }
        } else if (arg[0] == '-') {
            fprintf(stderr, "unknown option: %s\n", arg);
            return 1;
        } else {
            if (filename != NULL) {
                fprintf(stderr, "multiple input files specified: %s\n", arg);
                return 1;
            }

            filename = arg;
        }
    }

    if (filename == NULL) {
        fprintf(stderr, "no input file specified\n");
        return 1;
    }

    DataWin dw = {0};

    if (DataWin_loadFile(&dw, filename) != 0) {
        fprintf(stderr, "failed to load file: %s\n", filename);
        return 1;
    }

    if (DataWin_parse(&dw) != 0) {
        fprintf(stderr, "failed to parse file: %s\n", filename);
        DataWin_free(&dw);
        return 1;
    }

    printf("Loaded %zu bytes with %zu chunks\n",
           dw.file_size, dw.chunks.count);
    printf("Detected format version: %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
           dw.detected_format.major,
           dw.detected_format.minor,
           dw.detected_format.release,
           dw.detected_format.build);

    if (printGen8) GEN8_Print(&dw.gen8);
    if (printOptn) OPTN_Print(&dw.optn);
    if (printLang) LANG_Print(&dw.lang);
    if (printExtn) EXTN_Print(&dw.extn);
    if (printSond) SOND_Print(&dw.sond);
    if (printAgrp) AGRP_Print(&dw.agrp);
    if (printSprt) SPRT_Print(&dw.sprt);
    if (printBgnd) BGND_Print(&dw.bgnd);
    if (printPath) PATH_Print(&dw.path);

    DataWin_free(&dw);
    return 0;
}

void GEN8_Print(const Gen8 *g) {
    printf("GEN8:\n");
    printf("  isDebuggerDisabled: %s\n", g->isDebuggerDisabled ? "Yes" : "No");
    printf("  wadVersion: %" PRIu8 "\n", g->wadVersion);

    printf("  fileName: %s\n", g->fileName);
    printf("  config: %s\n", g->config);
    printf("  lastObj: %" PRIu32 "\n", g->lastObj);
    printf("  lastTile: %" PRIu32 "\n", g->lastTile);
    printf("  gameID: %" PRIu32 "\n", g->gameID);

    printf("  directPlayGuid: ");
    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) printf("-");
        printf("%02" PRIX8, g->directPlayGuid[i]);
    }
    printf("\n");

    printf("  name: %s\n", g->name);
    printf("  major: %" PRIu32 "\n", g->major);
    printf("  minor: %" PRIu32 "\n", g->minor);
    printf("  release: %" PRIu32 "\n", g->release);
    printf("  build: %" PRIu32 "\n", g->build);

    printf("  defaultWindowWidth: %" PRIu32 "\n", g->defaultWindowWidth);
    printf("  defaultWindowHeight: %" PRIu32 "\n", g->defaultWindowHeight);

    printf("  info: %" PRIu32 "\n", g->info);
    printf("  licenseCRC32: %" PRIx32 "\n", g->licenseCRC32);
    printf("  licenseMD5: ");
    for (int i = 0; i < 16; i++) {
        printf("%02" PRIX8 " ", g->licenseMD5[i]);
    }
    printf("\n");
    printf("  timestamp: %" PRIu64 "\n", g->timestamp);
    printf("  displayName: %s\n", g->displayName);
    printf("  activeTargets: %" PRIu64 "\n", g->activeTargets);
    printf("  functionClassifications: %" PRIu64 "\n", g->functionClassifications);
    printf("  steamAppID: %" PRIi32 "\n", g->steamAppID);
    printf("  debuggerPort: %" PRIu32 "\n", g->debuggerPort);
    printf("  roomOrderCount: %" PRIu32 "\n", g->roomOrderCount);
    printf("  gms2FPS: %f\n", g->gms2FPS);
}

void OPTN_Print(const Optn *o) {
    if (o == NULL) {
        return;
    }

    printf("OPTN:\n");
    printf("  shaderExtensionFlag:     %" PRId32 " (0x%08" PRIx32 ")\n",
           o->shaderExtensionFlag,
           (uint32_t)o->shaderExtensionFlag);

    printf("  shaderExtensionVersion:  %" PRId32 " (0x%08" PRIx32 ")\n",
           o->shaderExtensionVersion,
           (uint32_t)o->shaderExtensionVersion);

    printf("  info:                    0x%016" PRIx64 "\n", o->info);
    printf("    fullscreen:            %s\n", (o->info & INFO_FULLSCREEN) ? "true" : "false");
    printf("    interpolate_pixels:    %s\n", (o->info & INFO_INTERPOLATE_PIXELS) ? "true" : "false");
    printf("    use_new_audio:         %s\n", (o->info & INFO_USE_NEW_AUDIO) ? "true" : "false");
    printf("    no_border:             %s\n", (o->info & INFO_NO_BORDER) ? "true" : "false");
    printf("    show_cursor:           %s\n", (o->info & INFO_SHOW_CURSOR) ? "true" : "false");
    printf("    sizable:               %s\n", (o->info & INFO_SIZABLE) ? "true" : "false");
    printf("    stay_on_top:           %s\n", (o->info & INFO_STAY_ON_TOP) ? "true" : "false");
    printf("    change_resolution:     %s\n", (o->info & INFO_CHANGE_RESOLUTION) ? "true" : "false");
    printf("    no_buttons:            %s\n", (o->info & INFO_NO_BUTTONS) ? "true" : "false");
    printf("    screen_key:            %s\n", (o->info & INFO_SCREEN_KEY) ? "true" : "false");
    printf("    help_key:              %s\n", (o->info & INFO_HELP_KEY) ? "true" : "false");
    printf("    quit_key:              %s\n", (o->info & INFO_QUIT_KEY) ? "true" : "false");
    printf("    save_key:              %s\n", (o->info & INFO_SAVE_KEY) ? "true" : "false");
    printf("    screenshot_key:        %s\n", (o->info & INFO_SCREENSHOT_KEY) ? "true" : "false");
    printf("    close_sec:             %s\n", (o->info & INFO_CLOSE_SEC) ? "true" : "false");
    printf("    freeze:                %s\n", (o->info & INFO_FREEZE) ? "true" : "false");
    printf("    show_progress:         %s\n", (o->info & INFO_SHOW_PROGRESS) ? "true" : "false");
    printf("    load_transparent:      %s\n", (o->info & INFO_LOAD_TRANSPARENT) ? "true" : "false");
    printf("    scale_progress:        %s\n", (o->info & INFO_SCALE_PROGRESS) ? "true" : "false");
    printf("    display_errors:        %s\n", (o->info & INFO_DISPLAY_ERRORS) ? "true" : "false");
    printf("    write_errors:          %s\n", (o->info & INFO_WRITE_ERRORS) ? "true" : "false");
    printf("    abort_errors:          %s\n", (o->info & INFO_ABORT_ERRORS) ? "true" : "false");
    printf("    variable_errors:       %s\n", (o->info & INFO_VARIABLE_ERRORS) ? "true" : "false");
    printf("    creation_event_order:  %s\n", (o->info & INFO_CREATION_EVENT_ORDER) ? "true" : "false");

    printf("  scale:                   %" PRId32 "\n", o->scale);
    printf("  windowColor:             0x%08" PRIx32 "\n", o->windowColor);
    printf("  colorDepth:              %" PRIu32 "\n", o->colorDepth);
    printf("  resolution:              %" PRIu32 "\n", o->resolution);
    printf("  frequency:               %" PRIu32 "\n", o->frequency);
    printf("  vertexSync:              %" PRIu32 "\n", o->vertexSync);
    printf("  priority:                %" PRIu32 "\n", o->priority);
    printf("  backImage:               %" PRIu32 "\n", o->backImage);
    printf("  frontImage:              %" PRIu32 "\n", o->frontImage);
    printf("  loadImage:               %" PRIu32 "\n", o->loadImage);
    printf("  loadAlpha:               %" PRIu32 "\n", o->loadAlpha);

    printf("  constantCount:           %" PRIu32 "\n", o->constantCount);

    for (uint32_t i = 0; i < o->constantCount; i++) {
        printf("  constant[%" PRIu32 "]:\n", i);
        printf("    name:                 %s\n",
               o->constants[i].name ? o->constants[i].name : "(null)");
        printf("    value:                %s\n",
               o->constants[i].value ? o->constants[i].value : "(null)");
    }
}

void LANG_Print(const Lang *l) {
    if (l == NULL) {
        return;
    }

    printf("LANG:\n");
    printf("  unknown1:      %" PRIu32 "\n", l->unknown1);
    printf("  languageCount: %" PRIu32 "\n", l->languageCount);
    printf("  entryCount:    %" PRIu32 "\n", l->entryCount);

    printf("  entryIds:\n");
    if (l->entryCount == 0) {
        printf("    (none)\n");
    } else {
        for (uint32_t i = 0; i < l->entryCount; i++) {
            printf("    [%u] %s\n",
                   i,
                   l->entryIds[i] != NULL ? l->entryIds[i] : "(null)");
        }
    }

    printf("  languages:\n");
    if (l->languageCount == 0) {
        printf("    (none)\n");
        return;
    }

    for (uint32_t i = 0; i < l->languageCount; i++) {
        const Language *language = &l->languages[i];

        printf("    [%u]:\n", i);
        printf("      name:   %s\n",
               language->name != NULL ? language->name : "(null)");
        printf("      region: %s\n",
               language->region != NULL ? language->region : "(null)");

        printf("      entries:\n");

        if (language->entryCount == 0) {
            printf("        (none)\n");
            continue;
        }

        for (uint32_t j = 0; j < language->entryCount; j++) {
            printf("        [%u] %s\n",
                   j,
                   language->entries[j] != NULL
                       ? language->entries[j]
                       : "(null)");
        }
    }
}

void EXTN_Print(const Extn *e) {
    printf("EXTN:\n");
    printf("  extensionCount: %" PRIu32 "\n", e->count);

    for (uint32_t i = 0; i < e->count; i++) {
        const Extension *ext = &e->extensions[i];

        printf("  [%u] Extension:\n", i);
        printf("    folderName: %s\n", ext->folderName ? ext->folderName : "");
        printf("    name: %s\n", ext->name ? ext->name : "");
        printf("    className: %s\n", ext->className ? ext->className : "");
        printf("    fileCount: %" PRIu32 "\n", ext->fileCount);

        for (uint32_t j = 0; j < ext->fileCount; j++) {
            const ExtensionFile *file = &ext->files[j];

            printf("    [%u] File:\n", j);
            printf("      filename: %s\n",
                   file->filename ? file->filename : "");
            printf("      cleanupScript: %s\n",
                   file->cleanupScript ? file->cleanupScript : "");
            printf("      initScript: %s\n",
                   file->initScript ? file->initScript : "");
            printf("      kind: %" PRIu32 "\n", file->kind);
            printf("      functionCount: %" PRIu32 "\n",
                   file->functionCount);

            for (uint32_t k = 0; k < file->functionCount; k++) {
                const ExtensionFunction *func = &file->functions[k];

                printf("      [%u] Function:\n", k);
                printf("        name: %s\n",
                       func->name ? func->name : "");
                printf("        id: %" PRIu32 "\n", func->id);
                printf("        kind: %" PRIu32 "\n", func->kind);
                printf("        retType: %" PRIu32 "\n", func->retType);
                printf("        extName: %s\n",
                       func->extName ? func->extName : "");
                printf("        argumentCount: %" PRIu32 "\n",
                       func->argumentCount);

                if (func->argumentCount > 0) {
                    printf("        arguments: ");

                    for (uint32_t n = 0; n < func->argumentCount; n++) {
                        if (n > 0) printf(", ");
                        printf("%" PRIu32, func->arguments[n]);
                    }

                    printf("\n");
                }
            }
        }
    }
}

void SOND_Print(const Sond *s) {
    printf("SOND:\n");
    printf("  count: %" PRIu32 "\n", s->count);

    for (uint32_t i = 0; i < s->count; i++) {
        const Sound *snd = &s->sounds[i];

        printf("  sound[%" PRIu32 "]:\n", i);
        printf("    present: %s\n", snd->present ? "true" : "false");

        if (!snd->present) {
            continue;
        }

        printf("    name: %s\n", snd->name ? snd->name : "(null)");
        printf("    flags: 0x%" PRIx32 "\n", snd->flags);
        printf("    type: %s\n", snd->type ? snd->type : "(null)");
        printf("    file: %s\n", snd->file ? snd->file : "(null)");
        printf("    effects: 0x%" PRIx32 "\n", snd->effects);
        printf("    volume: %f\n", snd->volume);
        printf("    pan: %f\n", snd->pan);
        printf("    pitch: %f\n", snd->pitch);
        printf("    audioGroup: %" PRId32 "\n", snd->audioGroup);
        printf("    audioFile: %" PRId32 "\n", snd->audioFile);

        printf("    isEmbedded: %s\n",
               (snd->flags & AUDIO_ENTRY_FLAG_IS_EMBEDDED) != 0
                   ? "true"
                   : "false");

        printf("    isRegular: %s\n",
               (snd->flags & AUDIO_ENTRY_FLAG_REGULAR) != 0
                   ? "true"
                   : "false");
    }
}

void AGRP_Print(const Agrp *a) {
    printf("AGRP:\n");
    printf("  count: %" PRIu32 "\n", a->count);

    for (uint32_t i = 0; i < a->count; i++) {
        const AudioGroup *ag = &a->audioGroups[i];

        printf("  audioGroup[%" PRIu32 "]:\n", i);
        printf("    present: %s\n", ag->present ? "true" : "false");

        if (!ag->present) {
            continue;
        }

        printf("    name: %s\n", ag->name ? ag->name : "(null)");

        if (ag->path) {
            printf("    path: %s\n", ag->path);
        }
    }
}

void SPRT_Print(const Sprt *s) {
    printf("SPRT:\n");
    printf("  count: %" PRIu32 "\n", s->count);
    printf("  parsedCount: %" PRIu32 "\n", s->parsedCount);

    for (uint32_t i = 0; i < s->count; i++) {
        const Sprite *spr = &s->sprites[i];

        printf("  sprite[%" PRIu32 "]:\n", i);
        printf("    present: %s\n", spr->present ? "true" : "false");

        if (!spr->present) {
            continue;
        }

        printf("    name: %s\n", spr->name ? spr->name : "(null)");
        printf("    width: %" PRIu32 "\n", spr->width);
        printf("    height: %" PRIu32 "\n", spr->height);

        printf("    marginLeft: %" PRId32 "\n", spr->marginLeft);
        printf("    marginRight: %" PRId32 "\n", spr->marginRight);
        printf("    marginBottom: %" PRId32 "\n", spr->marginBottom);
        printf("    marginTop: %" PRId32 "\n", spr->marginTop);

        printf("    transparent: %s\n",
               spr->transparent ? "true" : "false");
        printf("    smooth: %s\n",
               spr->smooth ? "true" : "false");
        printf("    preload: %s\n",
               spr->preload ? "true" : "false");

        printf("    bboxMode: %" PRIu32 "\n", spr->bboxMode);
        printf("    sepMasks: %" PRIu32 "\n", spr->sepMasks);

        printf("    originX: %" PRId32 "\n", spr->originX);
        printf("    originY: %" PRId32 "\n", spr->originY);

        printf("    specialType: %s\n",
               spr->specialType ? "true" : "false");

        if (spr->specialType) {
            printf("    sVersion: %" PRIu32 "\n", spr->sVersion);
            printf("    sSpriteType: %" PRIu32 "\n", spr->sSpriteType);

            if (spr->sSpriteType == 0) {
                printf("    gms2PlaybackSpeed: %f\n",
                       spr->gms2PlaybackSpeed);
                printf("    gms2PlaybackSpeedType: %s\n",
                       spr->gms2PlaybackSpeedType ? "true" : "false");
            }
        }

        printf("    textureCount: %" PRId32 "\n", spr->textureCount);

        if (spr->textureCount > 0 && spr->tpagIndices != NULL) {
            printf("    tpagIndices:");
            for (uint32_t j = 0; j < spr->textureCount; j++) {
                printf(" %" PRId32, spr->tpagIndices[j]);
            }
            printf("\n");
        }

        printf("    maskWidth: %" PRIu32 "\n", spr->maskWidth);
        printf("    maskHeight: %" PRIu32 "\n", spr->maskHeight);
        printf("    maskOffsetX: %" PRId32 "\n", spr->maskOffsetX);
        printf("    maskOffsetY: %" PRId32 "\n", spr->maskOffsetY);
        printf("    maskCount: %" PRIu32 "\n", spr->maskCount);

        /*
         * Don't print the actual mask bytes by default. They can be
         * extremely large and are primarily useful for debugging the
         * parser itself.
         */
        printf("    masks: %s\n",
               spr->masks ? "present" : "null");

        if (spr->masks && spr->maskCount > 0) {
            printf("    maskDimensions: %" PRIu32 "x%" PRIu32 "\n",
                   spr->maskWidth,
                   spr->maskHeight);
        }

        printf("    nineSlice:\n");
        printf("      enabled: %s\n",
               spr->nineSliceEnabled ? "true" : "false");
        printf("      left: %" PRId32 "\n", spr->nsLeft);
        printf("      top: %" PRId32 "\n", spr->nsTop);
        printf("      right: %" PRId32 "\n", spr->nsRight);
        printf("      bottom: %" PRId32 "\n", spr->nsBottom);

        printf("      tileModes:");
        for (int j = 0; j < 5; j++) {
            printf(" %" PRIu8, spr->nsTileModes[j]);
        }
        printf("\n");
    }
}

void BGND_Print(const Bgnd *b) {
    printf("BGND:\n");
    printf("  count: %" PRIu32 "\n", b->count);

    for (uint32_t i = 0; i < b->count; i++) {
        const Background *bg = &b->backgrounds[i];

        printf("  background[%" PRIu32 "]:\n", i);
        printf("    present: %s\n", bg->present ? "true" : "false");

        if (!bg->present) {
            continue;
        }

        printf("    name: %s\n", bg->name ? bg->name : "(null)");
        printf("    smooth: %s\n", bg->smooth ? "true" : "false");
        printf("    preload: %s\n", bg->preload ? "true" : "false");
        printf("    tpagIndex: %" PRId32 "\n", bg->tpagIndex);

        if (bg->gms2UnknownAlways2 || bg->gms2TileWidth | bg->gms2TileHeight) {
            printf("    gms2UnknownAlways2: %" PRIu32 "\n",
                   bg->gms2UnknownAlways2);
            printf("    gms2TileWidth: %" PRIu32 "\n",
                   bg->gms2TileWidth);
            printf("    gms2TileHeight: %" PRIu32 "\n",
                   bg->gms2TileHeight);

            if (bg->gms2TileSeparationX != 0 || bg->gms2TileSeparationY != 0) {
                printf("    gms2TileSeparationX: %" PRIu32 "\n",
                       bg->gms2TileSeparationX);
                printf("    gms2TileSeparationY: %" PRIu32 "\n",
                       bg->gms2TileSeparationY);
            }

            printf("    gms2OutputBorderX: %" PRIu32 "\n",
                   bg->gms2OutputBorderX);
            printf("    gms2OutputBorderY: %" PRIu32 "\n",
                   bg->gms2OutputBorderY);
            printf("    gms2TileColumns: %" PRIu32 "\n",
                   bg->gms2TileColumns);
            printf("    gms2ItemsPerTileCount: %" PRIu32 "\n",
                   bg->gms2ItemsPerTileCount);
            printf("    gms2TileCount: %" PRIu32 "\n",
                   bg->gms2TileCount);
            printf("    gms2ExportedSpriteIndex: %" PRId32 "\n",
                   bg->gms2ExportedSpriteIndex);
            printf("    gms2FrameLength: %" PRId64 "\n",
                   bg->gms2FrameLength);

            uint64_t tileIdCount =
                (uint64_t) bg->gms2TileCount *
                (uint64_t) bg->gms2ItemsPerTileCount;

            printf("    gms2TileIds:");
            if (bg->gms2TileIds != NULL) {
                for (uint64_t j = 0; j < tileIdCount; j++) {
                    printf(" %" PRIu32, bg->gms2TileIds[j]);
                }
            }
            printf("\n");
        }
    }
}

void PATH_Print(const Path *p) {
    printf("PATH:\n");
    printf("  count: %" PRIu32 "\n", p->count);

    for (uint32_t i = 0; i < p->count; i++) {
        const GamePath *path = &p->paths[i];

        printf("  path[%" PRIu32 "]:\n", i);
        printf("    present: %s\n", path->present ? "true" : "false");

        if (!path->present) {
            continue;
        }

        printf("    name: %s\n", path->name ? path->name : "(null)");
        printf("    isSmooth: %s\n",
               path->isSmooth ? "true" : "false");
        printf("    isClosed: %s\n",
               path->isClosed ? "true" : "false");
        printf("    precision: %" PRIu32 "\n", path->precision);
        printf("    exists: %s\n",
               path->exists ? "true" : "false");

        printf("    pointCount: %" PRIu32 "\n", path->pointCount);

        for (uint32_t j = 0; j < path->pointCount; j++) {
            const PathPoint *point = &path->points[j];

            printf("    point[%" PRIu32 "]:\n", j);
            printf("      x: %f\n", point->x);
            printf("      y: %f\n", point->y);
            printf("      speed: %f\n", point->speed);
        }

        printf("    internalPointCount: %" PRIu32 "\n",
               path->internalPointCount);
        printf("    length: %f\n", path->length);

        if (path->internalPoints != NULL) {
            for (uint32_t j = 0; j < path->internalPointCount; j++) {
                const InternalPathPoint *point =
                    &path->internalPoints[j];

                printf("    internalPoint[%" PRIu32 "]:\n", j);
                printf("      x: %f\n", point->x);
                printf("      y: %f\n", point->y);
                printf("      speed: %f\n", point->speed);
            }
        }
    }
}