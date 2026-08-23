#include "gmdata.h"
#include "reader.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static int bump_version_from_candidate(DataWin *state, uint32_t major, uint32_t minor, uint32_t release, uint32_t build) {
    if (state == NULL) {
        return -1;
    }

    DataWin_bumpVersionTo(state, major, minor, release, build);
    return 0;
}

static int parse_gen8_version_from_reader(Reader *reader, DataWin *state, DetectedFormat *out) {
    if (reader == NULL || state == NULL || out == NULL) {
        return -1;
    }

    if (reader->size < 4U) {
        return -1;
    }

    uint8_t wadVersion = 0;
    read(&wadVersion, UInt8);
    read(&wadVersion, UInt8);

    if (wadVersion < 8U && reader->size <= 108U) {
        *out = (DetectedFormat){ 1U, 0U, 0U, 198U };
        return 0;
    }

    skip(2); // padding
    skip(sizeof(uint32_t)); // fileName offset
    skip(sizeof(uint32_t)); // config offset
    skip(sizeof(uint32_t)); // lastObj
    skip(sizeof(uint32_t)); // lastTile
    skip(sizeof(uint32_t)); // gameId
    skip(sizeof(uint8_t) * 16); // directPlayGuid
    skip(sizeof(uint32_t)); // versionName offset

    if (reader->cursor + 16U > reader->size) {
        logError("[parse_gen8_version_from_reader] Unexpected end of GEN8 chunk while reading version numbers\n");
        return -1;
    }

    uint32_t major = 0U;
    uint32_t minor = 0U;
    uint32_t release = 0U;
    uint32_t build = 0U;

    read(&major, UInt32);
    read(&minor, UInt32);
    read(&release, UInt32);
    read(&build, UInt32);

    *out = (DetectedFormat){ major, minor, release, build };
    (void)state;
    return 0;
}

static int detect_txtr_version(Reader *reader, DataWin *state) {
    if (reader == NULL || state == NULL) {
        return -1;
    }

    uint32_t *ptrs = NULL;
    uint32_t count = 0U;
    if (Reader_readPointerTable(reader, &ptrs, &count) != 0) {
        return -1;
    }
    if (count < 2U || ptrs[0] == 0U || ptrs[1] == 0U) {
        free(ptrs);
        return 0;
    }

    const uint32_t diff = ptrs[1] - ptrs[0];
    if (diff == 28U) {
        bump_version_from_candidate(state, 2022U, 9U, 0U, 0U);
    } else if (diff == 16U && !DataWin_isVersionAtLeast(state, 2022U, 3U, 0U, 0U)) {
        bump_version_from_candidate(state, 2022U, 3U, 0U, 0U);
    }

    free(ptrs);
    return 0;
}

static int detect_agrp_version(Reader *reader, DataWin *state) {
    if (reader == NULL || state == NULL) {
        return -1;
    }

    uint32_t *ptrs = NULL;
    uint32_t count = 0U;
    if (Reader_readPointerTable(reader, &ptrs, &count) != 0) {
        return -1;
    }
    if (count == 0U) {
        free(ptrs);
        return 0;
    }

    if (count >= 2U) {
        const uint32_t diff = ptrs[1] - ptrs[0];
        if (diff >= 8U) {
            bump_version_from_candidate(state, 2024U, 14U, 0U, 0U);
        }
    } else if (ptrs[0] != 0U) {
        const size_t saved_pos = reader->cursor;
        seek(ptrs[0]);
        const char *name = NULL;
        const char *path = NULL;
        if (Reader_readString(reader, state, &name) == 0 && Reader_readString(reader, state, &path) == 0) {
            if (strcmp(name, "audiogroup_default") == 0 && path != NULL) {
                bump_version_from_candidate(state, 2024U, 14U, 0U, 0U);
            }
            free((void *)name);
            free((void *)path);
        }
        seek(saved_pos);
    }

    free(ptrs);
    return 0;
}

static int detect_bgnd_version(Reader *reader, DataWin *state) {
    if (reader == NULL || state == NULL) {
        return -1;
    }

    uint32_t *ptrs = NULL;
    uint32_t count = 0U;
    if (Reader_readPointerTable(reader, &ptrs, &count) != 0) {
        return -1;
    }
    if (count == 0U || !DataWin_isVersionAtLeast(state, 2024U, 13U, 0U, 0U) || DataWin_isVersionAtLeast(state, 2024U, 14U, 1U, 0U)) {
        free(ptrs);
        return 0;
    }

    for (uint32_t i = 0; i < count; ++i) {
        if (ptrs[i] == 0U) {
            continue;
        }

        seek(ptrs[i] + (11U * 4U));
        uint32_t itemsPerTileCount = 0U;
        uint32_t tileCount = 0U;
        if (Reader_readUInt32(reader, &itemsPerTileCount) != 0 || Reader_readUInt32(reader, &tileCount) != 0) {
            free(ptrs);
            return -1;
        }

        size_t tpos = (size_t)ptrs[i] + (16U * 4U) + ((size_t)itemsPerTileCount * (size_t)tileCount * 4U);
        if (count >= 2U && i < count - 1U) {
            if ((tpos % 8U) != 0U) {
                tpos += 8U - (tpos % 8U);
            }
            if (tpos != (size_t)ptrs[i + 1U]) {
                bump_version_from_candidate(state, 2024U, 14U, 1U, 0U);
                break;
            }
        } else {
            if ((tpos % 16U) != 0U) {
                tpos += 16U - (tpos % 16U);
            }
            if (tpos != (size_t)reader->size) {
                bump_version_from_candidate(state, 2024U, 14U, 1U, 0U);
                break;
            }
        }
    }

    free(ptrs);
    return 0;
}

static int detect_func_version(Reader *reader, DataWin *state, size_t chunk_length) {
    if (reader == NULL || state == NULL || chunk_length == 0U) {
        return 0;
    }

    if (DataWin_isVersionAtLeast(state, 2024U, 8U, 0U, 0U)) {
        return 0;
    }

    const size_t funcChunkStart = reader->cursor;
    const size_t funcChunkEnd = funcChunkStart + chunk_length;
    uint32_t probeCount = 0U;
    read(&probeCount, UInt32);

    const size_t afterFunctions = reader->cursor + (size_t)probeCount * 12U;
    bool is2024_8 = false;
    if (afterFunctions == funcChunkEnd) {
        is2024_8 = true;
    } else if (funcChunkEnd > afterFunctions) {
        Reader_seek(reader, afterFunctions);
        int paddingBytesRead = 0;
        bool onlyPadding = true;
        while ((reader->cursor & 15U) != 0U) {
            if (reader->cursor >= funcChunkEnd) {
                onlyPadding = false;
                break;
            }
            uint8_t padByte = 0U;
            read(&padByte, UInt8);
            if (padByte != 0U) {
                onlyPadding = false;
                break;
            }
            paddingBytesRead++;
        }
        if (onlyPadding && reader->cursor == funcChunkEnd && (paddingBytesRead < 4 || state->code.count > 0)) {
            is2024_8 = true;
        }
    }

    if (is2024_8) {
        bump_version_from_candidate(state, 2024U, 8U, 0U, 0U);
    }

    seek(funcChunkStart);
    return 0;
}

static int detect_objt_version(Reader *reader, DataWin *state) {
    if (reader == NULL || state == NULL) {
        return -1;
    }

    uint32_t *ptrs = NULL;
    uint32_t count = 0U;
    if (Reader_readPointerTable(reader, &ptrs, &count) != 0) {
        return -1;
    }
    if (count == 0U || DataWin_isVersionAtLeast(state, 2022U, 5U, 0U, 0U)) {
        free(ptrs);
        return 0;
    }

    uint32_t probePtr = 0U;
    for (uint32_t i = 0; i < count; ++i) {
        if (ptrs[i] != 0U) {
            probePtr = ptrs[i];
            break;
        }
    }
    free(ptrs);

    if (probePtr == 0U) {
        return 0;
    }

    seek(probePtr + (16U * 4U));

    int32_t vertexCount = 0;
    read(&vertexCount, Int32);
    if (vertexCount < 0) {
        return 0;
    }

    const uint32_t skipCount = 12U + (uint32_t)vertexCount * 8U;
    const uint32_t newLocation = (uint32_t)reader->cursor + skipCount;
    bool isOldFormat = false;
    if (newLocation < reader->size) {
        skip((int)skipCount);
        uint32_t eventTypeCountA = 0U;
        uint32_t eventTypeCountB = 0U;
        read(&eventTypeCountA, UInt32);
        read(&eventTypeCountB, UInt32);
        if (eventTypeCountA == 0U && eventTypeCountB == 0U) {
            uint32_t firstSubEventPtr = 0U;
            read(&firstSubEventPtr, UInt32);
            if (firstSubEventPtr == reader->cursor + 14U * 4U) {
                isOldFormat = true;
            }
        }
}

    if (!isOldFormat) {
        bump_version_from_candidate(state, 2022U, 5U, 0U, 0U);
    }

    return 0;
}

static int detect_sond_version(Reader *reader, DataWin *state) {
    if (reader == NULL || state == NULL) {
        return -1;
    }

    uint32_t *ptrs = NULL;
    uint32_t count = 0U;
    if (Reader_readPointerTable(reader, &ptrs, &count) != 0) {
        return -1;
    }
    if (count == 0U || DataWin_isVersionAtLeast(state, 2024U, 6U, 0U, 0U) || !DataWin_isVersionAtLeast(state, 2023U, 2U, 0U, 0U)) {
        free(ptrs);
        return 0;
    }

    uint32_t soundPtrs[2] = { 0U, 0U };
    uint32_t soundCount = 0U;
    for (uint32_t i = 0; i < count; ++i) {
        if (ptrs[i] == 0U) {
            continue;
        }
        soundPtrs[soundCount++] = ptrs[i];
        if (soundCount >= 2U) {
            break;
        }
    }

    if (soundCount > 1U) {
        if (soundPtrs[0] + (4U * 9U) == soundPtrs[1] - 4U) {
            bump_version_from_candidate(state, 2024U, 6U, 0U, 0U);
        }
    } else if (soundCount == 1U) {
        const size_t savedPos = reader->cursor;
        const size_t probe = (size_t)soundPtrs[0] + (4U * 9U);
        if (Reader_seek(reader, probe) == 0) {
            uint32_t nextPtr = 0U;
            if (Reader_readUInt32(reader, &nextPtr) == 0 && nextPtr == 0U) {
                bump_version_from_candidate(state, 2024U, 6U, 0U, 0U);
            }
        }
        Reader_seek(reader, savedPos);
    }

    free(ptrs);
    return 0;
}

static int scan_chunk_table_for_version(const uint8_t *file_data, size_t file_size, DetectedFormat *out) {
    if (file_data == NULL || out == NULL) {
        return -1;
    }

    if (file_size < 12U || memcmp(file_data, "FORM", 4) != 0) {
        return -1;
    }

    DataWin state = {0};
    Reader re; Reader *reader = &re;
    Reader_init(reader, file_data, file_size, 0, "FORM");

    const uint32_t form_size = read_u32_le_at(file_data, file_size, 4U);
    const size_t end = 8U + (size_t)form_size;

    seek(8); // Skip "FORM" and form_size

    while (reader->cursor + 8U <= end && reader->cursor + 8U <= file_size) {
        char chunk_name[5] = {0};
        memcpy(chunk_name, file_data + reader->cursor, 4U);

        uint32_t chunk_length = 0U;
        read(&chunk_length, UInt32);

        const size_t chunk_offset = reader->cursor;
        const size_t chunk_end = chunk_offset + (size_t)chunk_length;
        if (chunk_end > end) {
            break;
        }

        if (memcmp(chunk_name, "ACRV", 4) == 0 || memcmp(chunk_name, "SEQN", 4) == 0 || memcmp(chunk_name, "TAGS", 4) == 0) {
            bump_version_from_candidate(&state, 2U, 3U, 0U, 0U);
        } else if (memcmp(chunk_name, "FEDS", 4) == 0) {
            bump_version_from_candidate(&state, 2U, 3U, 6U, 0U);
        } else if (memcmp(chunk_name, "FEAT", 4) == 0) {
            bump_version_from_candidate(&state, 2022U, 8U, 0U, 0U);
        } else if (memcmp(chunk_name, "UILR", 4) == 0) {
            bump_version_from_candidate(&state, 2024U, 13U, 0U, 0U);
        } else if (memcmp(chunk_name, "PSEM", 4) == 0 || memcmp(chunk_name, "PSYS", 4) == 0) {
            bump_version_from_candidate(&state, 2023U, 2U, 0U, 0U);
        }

        if (memcmp(chunk_name, "GEN8", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "GEN8");
            DetectedFormat gen8_version = {0U, 0U, 0U, 0U};
            if (parse_gen8_version_from_reader(&chunk_reader, &state, &gen8_version) == 0) {
                bump_version_from_candidate(&state, gen8_version.major, gen8_version.minor, gen8_version.release, gen8_version.build);
            }
        } else if (memcmp(chunk_name, "TXTR", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "TXTR");
            detect_txtr_version(&chunk_reader, &state);
        } else if (memcmp(chunk_name, "AGRP", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "AGRP");
            detect_agrp_version(&chunk_reader, &state);
        } else if (memcmp(chunk_name, "BGND", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "BGND");
            detect_bgnd_version(&chunk_reader, &state);
        } else if (memcmp(chunk_name, "FUNC", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "FUNC");
            detect_func_version(&chunk_reader, &state, chunk_length);
        } else if (memcmp(chunk_name, "OBJT", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "OBJT");
            detect_objt_version(&chunk_reader, &state);
        } else if (memcmp(chunk_name, "SOND", 4) == 0) {
            Reader chunk_reader;
            Reader_init(&chunk_reader, file_data + chunk_offset, chunk_length, chunk_offset, "SOND");
            detect_sond_version(&chunk_reader, &state);
        }

        if (Reader_seek(reader, chunk_end) != 0) {
            break;
        }
    }

    *out = state.detectedFormat;
    return 0;
}

int DataWin_detectVersionFromFile(const uint8_t *file_data, size_t file_size, DetectedFormat *out) {
    if (file_data == NULL || out == NULL) {
        return -1;
    }

    *out = (DetectedFormat){ 0U, 0U, 0U, 0U };
    return scan_chunk_table_for_version(file_data, file_size, out);
}
