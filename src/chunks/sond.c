#include "common.h"
#include "../datawin.h"

int Sound_parse(Reader *reader, DataWin *dw, Sound *sound);

int SOND_parse(DataWin *dw) {
    Chunk chunk = {0};
    Sond *s = &dw->sond;

    if (find_chunk(dw, "SOND", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length);

    uint32_t count;
    uint32_t* ptrs;
    if (Reader_readPointerTable(&reader, &ptrs, &count) != 0) return -1;
    s->count = count;

    if (count == 0) {
        s->sounds = NULL;
        free(ptrs);
        return 0;
    }

    if ( // Data version is between 2023.2.0.0 and 2024.6.0.0 (exclusive)
        DataWin_isVersionAtLeast(dw, 2023, 2, 0, 0) &&
        !DataWin_isVersionAtLeast(dw, 2024, 6, 0, 0)
    ) {
        uint32_t soundPtrs[2];
        uint32_t soundCount = 0;
        repeat(count, i) {
            if (ptrs[i] == 0) continue;
            soundPtrs[soundCount++] = ptrs[i];
            if (soundCount >= 2) break;
        }

        if (soundCount > 1) {
            if (soundPtrs[0] + (4 * 9) == soundPtrs[1] - 4) {
                DataWin_bumpVersionTo(dw, 2024, 6, 0, 0);
            }
        } else if (soundCount == 1) {
            size_t savedPos = reader.cursor;
            size_t probe = (size_t) (soundPtrs[0] + (4 * 9));
            assert((probe % 16) != 4);
            Reader_seek(&reader, probe);
            uint32_t nextPtr;
            if ((Reader_readUInt32(&reader, &nextPtr) == 0) && nextPtr == 0) {
                DataWin_bumpVersionTo(dw, 2024, 6, 0, 0);
            }
            Reader_seek(&reader, savedPos);
        }
    }

    s->sounds = (Sound*) malloc(sizeof(Sound) * count);
    if (s->sounds == NULL) {
        free(ptrs);
        return -1;
    }

    repeat(count, i) {
        if (ptrs[i] == 0) {
            memset(&s->sounds[i], 0, sizeof(Sound));
            continue;
        }

        Reader_seek(&reader, ptrs[i]);
        if (Sound_parse(&reader, dw, &s->sounds[i]) != 0) {
            free(ptrs);
            return -1;
        }
    }
    
    return 0;
}

int Sound_parse(Reader *reader, DataWin *dw, Sound *snd) {
    snd->present = true;
    Reader_readString(reader, dw, &snd->name);
    Reader_readUInt32(reader, &snd->flags);
    Reader_readString(reader, dw, &snd->type);
    Reader_readString(reader, dw, &snd->file);
    Reader_readUInt32(reader, &snd->effects);
    Reader_readFloat32(reader, &snd->volume);
    
    // Pre-WAD13 games store pan instead of pitch, and
    // stores the embedded flag as a separate boolean.
    if (dw->gen8.wadVersion <= 12) {
        bool embedded;

        Reader_readFloat32(reader, &snd->pan);
        Reader_readBool32(reader, &embedded);
        Reader_readInt32(reader, &snd->audioFile);

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
    Reader_readFloat32(reader, &snd->pitch);

    // AudioGroup or preload field at offset +28
    // For GMS 1.4.x (wadVersion >= 14) with Regular flag: resource_id
    if ((snd->flags & AUDIO_ENTRY_FLAG_REGULAR) == AUDIO_ENTRY_FLAG_REGULAR && dw->gen8.wadVersion >= 14) {
        Reader_readInt32(reader, &snd->audioGroup);
    } else {
        int32_t preload;
        Reader_readInt32(reader, &preload);
        snd->audioGroup = 0;
    }

    Reader_readInt32(reader, &snd->audioFile);

    return 0;
}

void Sound_free(Sound *snd) {
    if (snd == NULL) {
        return;
    }

    snd->present = false;
    snd->name = NULL;
    snd->type = NULL;
    snd->file = NULL;
}

void SOND_free(Sond *s) {
    if (s == NULL) {
        return;
    }

    for (uint32_t i = 0; i < s->count; ++i) {
        Sound_free(&s->sounds[i]);
    }

    free(s->sounds);
    s->sounds = NULL;
    s->count = 0;
}