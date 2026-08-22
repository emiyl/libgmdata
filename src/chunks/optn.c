#include "common.h"

int OPTN_parse(DataWin *dw) {
    Chunk chunk = {0};
    OptnChunk *o = &dw->optn;

    if (get_chunk(dw, "OPTN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "OPTN");

    read(&o->shaderExtensionFlag, Int32);
    bool newFormat = o->shaderExtensionFlag == (int32_t)0x80000000;

    if (newFormat) {
        read(&o->shaderExtensionVersion, Int32);
        read(&o->info, UInt64);
        read(&o->windowColor, UInt32);
        read(&o->colorDepth, UInt32);
        read(&o->resolution, UInt32);
        read(&o->frequency, UInt32);
        read(&o->vertexSync, UInt32);
        read(&o->priority, UInt32);
        read(&o->backImage, UInt32);
        read(&o->frontImage, UInt32);
        read(&o->loadImage, UInt32);
        read(&o->loadAlpha, UInt32);
    } else {
        Reader_skip(reader, -4);
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
        
        read(&fullscreen, Bool32);
        read(&interpolate_pixels, Bool32);
        read(&use_new_audio, Bool32);
        read(&no_border, Bool32);
        read(&show_cursor, Bool32);
        read(&o->scale, Int32);
        read(&sizable, Bool32);
        read(&stay_on_top, Bool32);
        read(&o->windowColor, UInt32);
        read(&change_resolution, Bool32);
        read(&o->colorDepth, UInt32);
        read(&o->resolution, UInt32);
        read(&o->frequency, UInt32);
        read(&no_buttons, Bool32);
        read(&o->vertexSync, UInt32);
        read(&screen_key, Bool32);
        read(&help_key, Bool32);
        read(&quit_key, Bool32);
        read(&save_key, Bool32);
        read(&screenshot_key, Bool32);
        read(&close_sec, Bool32);
        read(&o->priority, UInt32);
        read(&freeze, Bool32);
        read(&show_progress, Bool32);
        read(&o->backImage, UInt32);
        read(&o->frontImage, UInt32);
        read(&o->loadImage, UInt32);
        read(&load_transparent, Bool32);
        read(&o->loadAlpha, UInt32);
        read(&scale_progress, Bool32);
        read(&display_errors, Bool32);
        read(&write_errors, Bool32);
        read(&abort_errors, Bool32);
        read(&variable_errors, Bool32);
        read(&creation_event_order, Bool32);

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

    read(&o->constantCount, UInt32);

    if (o->constantCount == 0) {
        o->constants = NULL;
        return 0;
    }

    o->constants = (OptnConstant *)safeMalloc(o->constantCount * sizeof(OptnConstant));

    repeat(o->constantCount, i) {
        readString(&o->constants[i].name, dw);
        readString(&o->constants[i].value, dw);
    }

    return 0;
}

int OPTN_free(OptnChunk *o) {
    if (o == NULL) {
        return -1;
    }

    if (o->constants != NULL) {
        for (uint32_t i = 0; i < o->constantCount; ++i) {
            free((void *)o->constants[i].name);
            free((void *)o->constants[i].value);
        }
        free(o->constants);
        o->constants = NULL;
    }

    o->constantCount = 0;
    return 0;
}