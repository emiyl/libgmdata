#include "gen8.h"

static void GEN8_ParseRoomOrder(Reader *reader, Gen8 *g) {
    Reader_read_u32(reader, &g->roomOrderCount);

    if (g->roomOrderCount == 0) {
        g->roomOrder = NULL;
        return;
    }

    g->roomOrder = malloc(g->roomOrderCount * sizeof(*g->roomOrder));

    repeat(g->roomOrderCount, i) {
        Reader_read_i32(reader, &g->roomOrder[i]);
    }
}

int GEN8_Parse(DataWin *dw) {
    Chunk chunk = {0};
    Gen8* g = &dw->gen8;

    if(find_chunk(dw, "GEN8", &chunk) != 0) return -1;
    if(chunk.offset + chunk.length > dw->file_size) return -1;

    const bool isCompactWad8 = g->wadVersion < 8 && chunk.length <= 108;
    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length);

    // isDebuggerDisabled and wadVersion
    Reader_read_u8(&reader, &g->isDebuggerDisabled);
    Reader_read_u8(&reader, &g->wadVersion);
    Reader_skip(&reader, 2); // padding

    Reader_read_string(&reader, dw, &g->fileName);

    if (isCompactWad8) {
        g->config = "";
    } else {
        Reader_read_string(&reader, dw, &g->config);
    }

    Reader_read_u32(&reader, &g->lastObj);
    Reader_read_u32(&reader, &g->lastTile);
    Reader_read_u32(&reader, &g->gameID);

    Reader_read_bytes(&reader, g->directPlayGuid, sizeof(g->directPlayGuid));

    if (isCompactWad8) {
        // Compact WAD8 has no name/version fields.
        g->name = "";
        g->major = 1;
        g->minor = 0;
        g->release = 0;
        g->build = 198;
    } else {
        Reader_read_string(&reader, dw, &g->name);
        Reader_read_u32(&reader, &g->major);
        Reader_read_u32(&reader, &g->minor);
        Reader_read_u32(&reader, &g->release);
        Reader_read_u32(&reader, &g->build);
    }

    Reader_read_u32(&reader, &g->defaultWindowWidth);
    Reader_read_u32(&reader, &g->defaultWindowHeight);
    Reader_read_u32(&reader, &g->info);
    Reader_read_u32(&reader, &g->licenseCRC32);
    Reader_read_bytes(&reader, g->licenseMD5, sizeof(g->licenseMD5));

    // Compact WAD8 has a slightly different tail
    if (isCompactWad8) {
        uint32_t timestamp;
        Reader_read_u32(&reader, &timestamp);
        g->timestamp = (uint64_t)timestamp;

        Reader_skip(&reader, 4); // gap at offset 72

        g->displayName = "";
        g->activeTargets = 0;
        g->functionClassifications = 0;
        g->steamAppID = 0;
        g->debuggerPort = 0;

        GEN8_ParseRoomOrder(&reader, g);
        return 0;
    }

    // WAD8 < 12
    if (g->wadVersion < 12) {
        int32_t timestamp;
        Reader_read_i32(&reader, &timestamp);
        g->timestamp = (uint64_t)(int64_t)timestamp;

        Reader_skip(&reader, 4); // padding at body + 0x60

        if (g->wadVersion >= 9) {
            Reader_read_string(&reader, dw, &g->displayName);
        } else {
            g->displayName = "";
        }

        if (g->wadVersion >= 11) {
            Reader_read_u64(&reader, &g->activeTargets);
        } else {
            g->activeTargets = 0;
        }

        if (g->wadVersion >= 12) {
            Reader_read_u64(&reader, &g->functionClassifications);
        } else {
            g->functionClassifications = 0;
        }

        g->steamAppID = 0;
        g->debuggerPort = 0;

        GEN8_ParseRoomOrder(&reader, g);
        return 0;
    }

    // WAD8 >= 12
    Reader_read_u64(&reader, &g->timestamp);
    Reader_read_string(&reader, dw, &g->displayName);
    Reader_read_u64(&reader, &g->activeTargets);
    Reader_read_u64(&reader, &g->functionClassifications);
    Reader_read_i32(&reader, &g->steamAppID);

    if (g->wadVersion >= 14) {
        Reader_read_u32(&reader, &g->debuggerPort);
    } else {
        g->debuggerPort = 0;
    }

    GEN8_ParseRoomOrder(&reader, g);

    // GMS2+ fields
    if (g->major >= 2) {
        Reader_skip(&reader, 8);      // firstRandom
        Reader_skip(&reader, 8 * 4);  // four random entries

        Reader_read_f32(&reader, &g->gms2FPS);

        Reader_skip(&reader, 4);      // AllowStatistics
        Reader_skip(&reader, 16);     // GameGUID
    }

    return 0;
}

void GEN8_Bytedump(DataWin *dw) {
    Chunk chunk = {0};

    assert(find_chunk(dw, "GEN8", &chunk) == 0);
    assert(chunk.offset + chunk.length <= dw->file_size);

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length);

    printf("GEN8 Bytedump:\n");

    repeat(chunk.length, i) {
        uint8_t byte;
        Reader_read_u8(&reader, &byte);

        if (i % 16 == 0)
            printf("[%02x] ", i);
        printf("%02X", byte);
        if (i % 4 == 3)
            printf(" ");
        if (i % 16 == 15)
            printf("\n");
    }

    if (chunk.length % 16 != 0)
        printf("\n");
}