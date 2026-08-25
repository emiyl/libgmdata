#include "common.h"

static int RoomOrder_parse(Reader *reader, Gen8Chunk *g);

int GEN8_parse(DataWin *dw) {
    Chunk chunk = {0};
    Gen8Chunk* g = &dw->gen8;

    if(get_chunk(dw, "GEN8", &chunk) != 0) return -1;
    if(chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader re; Reader *reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "GEN8");
    

    // isDebuggerDisabled and wadVersion
    read(&g->isDebuggerDisabled, UInt8);
    read(&g->wadVersion, UInt8);
    
    const bool isCompactWad8 = g->wadVersion < 8 && chunk.length <= 108;
    Reader_skip(reader, 2); // padding

    readString(&g->fileName, dw);

    if (isCompactWad8) {
        g->config = NULL;
    } else {
        readString(&g->config, dw);
    }

    read(&g->lastObj, UInt32);
    read(&g->lastTile, UInt32);
    read(&g->gameID, UInt32);

    Reader_readBytes(reader, g->directPlayGuid, sizeof(g->directPlayGuid));

    if (isCompactWad8) {
        // Compact WAD8 has no name/version fields.
        g->name = NULL;
        g->major = 1;
        g->minor = 0;
        g->release = 0;
        g->build = 198;
    } else {
        readString(&g->name, dw);
        read(&g->major, UInt32);
        read(&g->minor, UInt32);
        read(&g->release, UInt32);
        read(&g->build, UInt32);
    }

    read(&g->defaultWindowWidth, UInt32);
    read(&g->defaultWindowHeight, UInt32);
    read(&g->info, UInt32);
    read(&g->licenseCRC32, UInt32);
    Reader_readBytes(reader, g->licenseMD5, sizeof(g->licenseMD5));

    // Compact WAD8 has a slightly different tail
    if (isCompactWad8) {
        uint32_t timestamp;
        read(&timestamp, UInt32);
        g->timestamp = (uint64_t)timestamp;

        Reader_skip(reader, 4); // gap at offset 72

        g->displayName = "";
        g->activeTargets = 0;
        g->functionClassifications = 0;
        g->steamAppID = 0;
        g->debuggerPort = 0;

        RoomOrder_parse(reader, g);
        return 0;
    }

    // WAD8 < 12
    if (g->wadVersion < 12) {
        int32_t timestamp;
        read(&timestamp, Int32);
        g->timestamp = (uint64_t)(int64_t)timestamp;

        Reader_skip(reader, 4); // padding at body + 0x60

        if (g->wadVersion >= 9) {
            readString(&g->displayName, dw);
        } else {
            g->displayName = NULL;
        }

        if (g->wadVersion >= 11) {
            read(&g->activeTargets, UInt64);
        } else {
            g->activeTargets = 0;
        }

        if (g->wadVersion >= 12) {
            read(&g->functionClassifications, UInt64);
        } else {
            g->functionClassifications = 0;
        }

        g->steamAppID = 0;
        g->debuggerPort = 0;

        RoomOrder_parse(reader, g);
        return 0;
    }

    // WAD8 >= 12
    read(&g->timestamp, UInt64);
    readString(&g->displayName, dw);
    read(&g->activeTargets, UInt64);
    read(&g->functionClassifications, UInt64);
    read(&g->steamAppID, Int32);

    if (g->wadVersion >= 14) {
        read(&g->debuggerPort, UInt32);
    } else {
        g->debuggerPort = 0;
    }

    RoomOrder_parse(reader, g);

    // GMS2+ fields
    if (g->major >= 2) {
        Reader_skip(reader, 8);      // firstRandom
        Reader_skip(reader, 8 * 4);  // four random entries

        read(&g->gms2FPS, Float32);

        Reader_skip(reader, 4);      // AllowStatistics
        Reader_skip(reader, 16);     // GameGUID
    }

    return 0;
}

static int RoomOrder_parse(Reader *reader, Gen8Chunk *g) {
    read(&g->roomOrderCount, UInt32);

    if (g->roomOrderCount == 0) {
        g->roomOrder = NULL;
        return 0;
    }

    g->roomOrder = safeMalloc(g->roomOrderCount * sizeof(*g->roomOrder));

    repeat(g->roomOrderCount, i) {
        read(&g->roomOrder[i], Int32);
    }
    return 0;
}

static int RoomOrder_free(Gen8Chunk *g) {
    if (g->roomOrder) {
        free(g->roomOrder);
        g->roomOrder = NULL;
    }
    g->roomOrderCount = 0;
    return 0;
}

int GEN8_free(Gen8Chunk *g) {
    if (g->fileName) free((void*)g->fileName);
    if (g->config) free((void*)g->config);
    if (g->name) free((void*)g->name);
    if (g->displayName) free((void*)g->displayName);
    RoomOrder_free(g);
    return 0;
}