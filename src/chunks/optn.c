#include "optn.h"

int OPTN_parse(DataWin *dw) {
    Chunk chunk = {0};
    Optn *o = &dw->optn;

    if (find_chunk(dw, "OPTN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length);

    Reader_readInt32(&reader, &o->shaderExtensionFlag);
    bool newFormat = o->shaderExtensionFlag == (int32_t)0x80000000;

    if (newFormat) {
        Reader_readInt32(&reader, &o->shaderExtensionVersion);
        Reader_readUInt64(&reader, &o->info);
        Reader_readUInt32(&reader, &o->windowColor);
        Reader_readUInt32(&reader, &o->colorDepth);
        Reader_readUInt32(&reader, &o->resolution);
        Reader_readUInt32(&reader, &o->frequency);
        Reader_readUInt32(&reader, &o->vertexSync);
        Reader_readUInt32(&reader, &o->priority);
        Reader_readUInt32(&reader, &o->backImage);
        Reader_readUInt32(&reader, &o->frontImage);
        Reader_readUInt32(&reader, &o->loadImage);
        Reader_readUInt32(&reader, &o->loadAlpha);
    } else {
        Reader_skip(&reader, -4);
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
        
        Reader_readBool32(&reader, &fullscreen);
        Reader_readBool32(&reader, &interpolate_pixels);
        Reader_readBool32(&reader, &use_new_audio);
        Reader_readBool32(&reader, &no_border);
        Reader_readBool32(&reader, &show_cursor);
        Reader_readInt32(&reader, &o->scale);
        Reader_readBool32(&reader, &sizable);
        Reader_readBool32(&reader, &stay_on_top);
        Reader_readUInt32(&reader, &o->windowColor);
        Reader_readBool32(&reader, &change_resolution);
        Reader_readUInt32(&reader, &o->colorDepth);
        Reader_readUInt32(&reader, &o->resolution);
        Reader_readUInt32(&reader, &o->frequency);
        Reader_readBool32(&reader, &no_buttons);
        Reader_readUInt32(&reader, &o->vertexSync);
        Reader_readBool32(&reader, &screen_key);
        Reader_readBool32(&reader, &help_key);
        Reader_readBool32(&reader, &quit_key);
        Reader_readBool32(&reader, &save_key);
        Reader_readBool32(&reader, &screenshot_key);
        Reader_readBool32(&reader, &close_sec);
        Reader_readUInt32(&reader, &o->priority);
        Reader_readBool32(&reader, &freeze);
        Reader_readBool32(&reader, &show_progress);
        Reader_readUInt32(&reader, &o->backImage);
        Reader_readUInt32(&reader, &o->frontImage);
        Reader_readUInt32(&reader, &o->loadImage);
        Reader_readBool32(&reader, &load_transparent);
        Reader_readUInt32(&reader, &o->loadAlpha);
        Reader_readBool32(&reader, &scale_progress);
        Reader_readBool32(&reader, &display_errors);
        Reader_readBool32(&reader, &write_errors);
        Reader_readBool32(&reader, &abort_errors);
        Reader_readBool32(&reader, &variable_errors);
        Reader_readBool32(&reader, &creation_event_order);

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

    Reader_readUInt32(&reader, &o->constantCount);

    if (o->constantCount == 0) {
        o->constants = NULL;
        return 0;
    }

    o->constants = (OptnConstant *)malloc(o->constantCount * sizeof(OptnConstant));

    repeat(o->constantCount, i) {
        Reader_readString(&reader, dw, &o->constants[i].name);
        Reader_readString(&reader, dw, &o->constants[i].value);
    }

    return 0;
}

void OPTN_free(Optn *o) {
    if (o == NULL) {
        return;
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
}