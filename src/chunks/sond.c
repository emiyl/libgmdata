#include "common.h"

static int SOND_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData);
static int SOND_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData);
int SOND_parse(DataWin *dw) {
    Chunk chunk = {0};
    SondChunk *s = &dw->sond;

    if (get_chunk(dw, "SOND", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "SOND");

    uint32_t* ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &s->count) != 0) return -1;

    if (s->count == 0) {
        s->sounds = NULL;
        free(ptrs);
        return 0;
    }
    
    Reader_parsePointerTable(
        &reader, dw,
        ptrs, s->count,
        (void **)&s->sounds, sizeof(Sound),
        NULL,
        SOND_pointerTable_parse,
        SOND_pointerTable_missingHandler,
        NULL
    );

    free(ptrs);
    return 0;
}

static int Sound_parse(Reader *reader, DataWin *dw, Sound *snd) {
    snd->present = true;

    readString(&snd->name, dw);
    read(&snd->flags, UInt32);
    readString(&snd->type, dw);
    readString(&snd->file, dw);
    read(&snd->effects, UInt32);
    read(&snd->volume, Float32);
    
    // Pre-WAD13 games store pan instead of pitch, and
    // stores the embedded flag as a separate boolean.
    if (dw->gen8.wadVersion <= 12) {
        bool embedded;

        read(&snd->pan, Float32);
        read(&embedded, Bool32);
        read(&snd->audioFile, Int32);

        if (embedded) {
            snd->flags |= AUDIO_ENTRY_FLAG_IS_EMBEDDED;
        } else {
            snd->flags &= ~AUDIO_ENTRY_FLAG_IS_EMBEDDED;
        }

        snd->pitch = 1.0f;
        snd->audioGroup = 0;
        
        return 0;
    }

    snd->pan = 0.0f;
    read(&snd->pitch, Float32);

    // AudioGroup or preload field at offset +28
    // For GMS 1.4.x (wadVersion >= 14) with Regular flag: resource_id
    if ((snd->flags & AUDIO_ENTRY_FLAG_REGULAR) == AUDIO_ENTRY_FLAG_REGULAR && dw->gen8.wadVersion >= 14) {
        read(&snd->audioGroup, Int32);
    } else {
        int32_t preload;
        read(&preload, Int32);
        snd->audioGroup = 0;
    }

    read(&snd->audioFile, Int32);

    return 0;
}

static int SOND_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)extraData; // Unused parameter
    return Sound_parse(reader, dw, (Sound *)out);
}
static int SOND_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void* extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[SOND_pointerTable_missingHandler] Sound pointer is missing, initializing default values.\n");

    Sound *snd = (Sound *)out;
    snd->present = false;
    snd->name = NULL;
    snd->type = NULL;
    snd->file = NULL;
    snd->flags = 0;
    snd->effects = 0;
    snd->volume = 1.0f;
    snd->pan = 0.0f;
    snd->pitch = 1.0f;
    snd->audioGroup = 0;
    snd->audioFile = 0;

    return 0;
}

static int Sound_free(Sound *snd) {
    if (snd == NULL) {
        return -1;
    }

    snd->present = false;
    snd->name = NULL;
    snd->type = NULL;
    snd->file = NULL;
    return 0;
}

int SOND_free(SondChunk *s) {
    if (s == NULL) {
        return -1;
    }

    int result = 0;
    for (uint32_t i = 0; i < s->count; ++i) {
        if (Sound_free(&s->sounds[i])) {
            logWarn("[SOND_free] Failed to free Sound at index %u\n", i);
            result = -1;
        }
    }

    free(s->sounds);
    s->sounds = NULL;
    s->count = 0;
    return result;
}