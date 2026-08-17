#include "optn.h"

#include "../reader.h"
#include "../types.h"
#include "../chunks.h"
#include "../utils.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

int OPTN_Parse(DataWin *dw) {
    Chunk chunk = {0};
    Optn *o = &dw->optn;

    if (find_chunk(dw, "OPTN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    reader_init(&reader, base, chunk.length);

    reader_read_i32_le(&reader, &o->shaderExtensionFlag);
    bool newFormat = o->shaderExtensionFlag == (int32_t)0x80000000;

    if (newFormat) {
        reader_read_i32_le(&reader, &o->shaderExtensionVersion);
        reader_read_u64_le(&reader, &o->info);
        reader_read_u32_le(&reader, &o->windowColor);
        reader_read_u32_le(&reader, &o->colorDepth);
        reader_read_u32_le(&reader, &o->resolution);
        reader_read_u32_le(&reader, &o->frequency);
        reader_read_u32_le(&reader, &o->vertexSync);
        reader_read_u32_le(&reader, &o->priority);
        reader_read_u32_le(&reader, &o->backImage);
        reader_read_u32_le(&reader, &o->frontImage);
        reader_read_u32_le(&reader, &o->loadImage);
        reader_read_u32_le(&reader, &o->loadAlpha);
    } else {
        reader_skip(&reader, -4);
        o->info = 0;

        bool fullscreen;
        bool interpolate_pixels;
        bool use_new_audio;
        bool no_border;
        bool show_cursor;
        bool sizable;
        bool stay_on_top;
        bool change_resolution;
        bool no_buttons;
        bool screen_key;
        bool help_key;
        bool quit_key;
        bool save_key;
        bool screenshot_key;
        bool close_sec;
        bool freeze;
        bool show_progress;
        bool load_transparent;
        bool scale_progress;
        bool display_errors;
        bool write_errors;
        bool abort_errors;
        bool variable_errors;
        bool creation_event_order;
        
        reader_read_b32_le(&reader, &fullscreen);
        reader_read_b32_le(&reader, &interpolate_pixels);
        reader_read_b32_le(&reader, &use_new_audio);
        reader_read_b32_le(&reader, &no_border);
        reader_read_b32_le(&reader, &show_cursor);
        reader_read_i32_le(&reader, &o->scale);
        reader_read_b32_le(&reader, &sizable);
        reader_read_b32_le(&reader, &stay_on_top);
        reader_read_u32_le(&reader, &o->windowColor);
        reader_read_b32_le(&reader, &change_resolution);
        reader_read_u32_le(&reader, &o->colorDepth);
        reader_read_u32_le(&reader, &o->resolution);
        reader_read_u32_le(&reader, &o->frequency);
        reader_read_b32_le(&reader, &no_buttons);
        reader_read_u32_le(&reader, &o->vertexSync);
        reader_read_b32_le(&reader, &screen_key);
        reader_read_b32_le(&reader, &help_key);
        reader_read_b32_le(&reader, &quit_key);
        reader_read_b32_le(&reader, &save_key);
        reader_read_b32_le(&reader, &screenshot_key);
        reader_read_b32_le(&reader, &close_sec);
        reader_read_u32_le(&reader, &o->priority);
        reader_read_b32_le(&reader, &freeze);
        reader_read_b32_le(&reader, &show_progress);
        reader_read_u32_le(&reader, &o->backImage);
        reader_read_u32_le(&reader, &o->frontImage);
        reader_read_u32_le(&reader, &o->loadImage);
        reader_read_b32_le(&reader, &load_transparent);
        reader_read_u32_le(&reader, &o->loadAlpha);
        reader_read_b32_le(&reader, &scale_progress);
        reader_read_b32_le(&reader, &display_errors);
        reader_read_b32_le(&reader, &write_errors);
        reader_read_b32_le(&reader, &abort_errors);
        reader_read_b32_le(&reader, &variable_errors);
        reader_read_b32_le(&reader, &creation_event_order);

        if (fullscreen)            o->info |= INFO_FULLSCREEN;
        if (interpolate_pixels)    o->info |= INFO_INTERPOLATE_PIXELS;
        if (use_new_audio)         o->info |= INFO_USE_NEW_AUDIO;
        if (no_border)             o->info |= INFO_NO_BORDER;
        if (show_cursor)           o->info |= INFO_SHOW_CURSOR;
        if (sizable)               o->info |= INFO_SIZABLE;
        if (stay_on_top)           o->info |= INFO_STAY_ON_TOP;
        if (change_resolution)     o->info |= INFO_CHANGE_RESOLUTION;
        if (no_buttons)            o->info |= INFO_NO_BUTTONS;
        if (screen_key)            o->info |= INFO_SCREEN_KEY;
        if (help_key)              o->info |= INFO_HELP_KEY;
        if (quit_key)              o->info |= INFO_QUIT_KEY;
        if (save_key)              o->info |= INFO_SAVE_KEY;
        if (screenshot_key)        o->info |= INFO_SCREENSHOT_KEY;
        if (close_sec)             o->info |= INFO_CLOSE_SEC;
        if (freeze)                o->info |= INFO_FREEZE;
        if (show_progress)         o->info |= INFO_SHOW_PROGRESS;
        if (load_transparent)      o->info |= INFO_LOAD_TRANSPARENT;
        if (scale_progress)        o->info |= INFO_SCALE_PROGRESS;
        if (display_errors)        o->info |= INFO_DISPLAY_ERRORS;
        if (write_errors)          o->info |= INFO_WRITE_ERRORS;
        if (abort_errors)          o->info |= INFO_ABORT_ERRORS;
        if (variable_errors)       o->info |= INFO_VARIABLE_ERRORS;
        if (creation_event_order)  o->info |= INFO_CREATION_EVENT_ORDER;
    }

    // Constants SimpleList (absent on WAD8)
    if (dw->gen8.wadVersion <= 8) {
        o->constantCount = 0;
        o->constants = NULL;
        return 0;
    }

    reader_read_u32_le(&reader, &o->constantCount);

    if (o->constantCount == 0) {
        o->constants = NULL;
        return 0;
    }

    o->constants = (OptnConstant *)malloc(o->constantCount * sizeof(OptnConstant));

    repeat(o->constantCount, i) {
        reader_read_string(&reader, dw, &o->constants[i].name);
        reader_read_string(&reader, dw, &o->constants[i].value);
    }

    return 0;
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