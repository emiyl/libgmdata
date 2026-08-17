#include "../include/datawin.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

void GEN8_Print(const Gen8 *g);
void OPTN_Print(const Optn *o);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s [options] <data.win>\n"
            "\n"
            "options:\n"
            "  --print <name>    Print a section\n"
            "                    gen8, optn, or *\n",
            argv[0]);
        return 1;
    }

    const char *filename = NULL;
    bool printGen8 = false;
    bool printOptn = false;

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
            } else if (strcmp(name, "gen8") == 0) {
                printGen8 = true;
            } else if (strcmp(name, "optn") == 0) {
                printOptn = true;
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

    if (load_file(&dw, filename) != 0) {
        fprintf(stderr, "failed to load file: %s\n", filename);
        return 1;
    }

    if (parse(&dw) != 0) {
        fprintf(stderr, "failed to parse file: %s\n", filename);
        datawin_free(&dw);
        return 1;
    }

    printf("Loaded %zu bytes with %zu chunks\n",
           dw.file_size, dw.chunks.count);

    if (printGen8) {
        GEN8_Print(&dw.gen8);
    }

    if (printOptn) {
        OPTN_Print(&dw.optn);
    }

    datawin_free(&dw);
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