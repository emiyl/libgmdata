#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../types.h"
#include "../chunks.h"
#include "../reader.h"
#include "../utils.h"

static void GEN8_ParseRoomOrder(Reader *reader, Gen8 *g) {
    reader_read_u32_le(reader, &g->roomOrderCount);

    if (g->roomOrderCount == 0) {
        g->roomOrder = NULL;
        return;
    }

    g->roomOrder = malloc(g->roomOrderCount * sizeof(*g->roomOrder));

    repeat(g->roomOrderCount, i) {
        reader_read_i32_le(reader, &g->roomOrder[i]);
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
    reader_init(&reader, base, chunk.length);

    // isDebuggerDisabled and wadVersion
    reader_read_u8(&reader, &g->isDebuggerDisabled);
    reader_read_u8(&reader, &g->wadVersion);
    reader_skip(&reader, 2); // padding

    reader_read_string(&reader, dw, &g->fileName);

    if (isCompactWad8) {
        g->config = "";
    } else {
        reader_read_string(&reader, dw, &g->config);
    }

    reader_read_u32_le(&reader, &g->lastObj);
    reader_read_u32_le(&reader, &g->lastTile);
    reader_read_u32_le(&reader, &g->gameID);

    reader_read_bytes(&reader, g->directPlayGuid, sizeof(g->directPlayGuid));

    if (isCompactWad8) {
        // Compact WAD8 has no name/version fields.
        g->name = "";
        g->major = 1;
        g->minor = 0;
        g->release = 0;
        g->build = 198;
    } else {
        reader_read_string(&reader, dw, &g->name);
        reader_read_u32_le(&reader, &g->major);
        reader_read_u32_le(&reader, &g->minor);
        reader_read_u32_le(&reader, &g->release);
        reader_read_u32_le(&reader, &g->build);
    }

    reader_read_u32_le(&reader, &g->defaultWindowWidth);
    reader_read_u32_le(&reader, &g->defaultWindowHeight);
    reader_read_u32_le(&reader, &g->info);
    reader_read_u32_le(&reader, &g->licenseCRC32);
    reader_read_bytes(&reader, g->licenseMD5, sizeof(g->licenseMD5));

    // Compact WAD8 has a slightly different tail
    if (isCompactWad8) {
        uint32_t timestamp;
        reader_read_u32_le(&reader, &timestamp);
        g->timestamp = (uint64_t)timestamp;

        reader_skip(&reader, 4); // gap at offset 72

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
        reader_read_i32_le(&reader, &timestamp);
        g->timestamp = (uint64_t)(int64_t)timestamp;

        reader_skip(&reader, 4); // padding at body + 0x60

        if (g->wadVersion >= 9) {
            reader_read_string(&reader, dw, &g->displayName);
        } else {
            g->displayName = "";
        }

        if (g->wadVersion >= 11) {
            reader_read_u64_le(&reader, &g->activeTargets);
        } else {
            g->activeTargets = 0;
        }

        if (g->wadVersion >= 12) {
            reader_read_u64_le(&reader, &g->functionClassifications);
        } else {
            g->functionClassifications = 0;
        }

        g->steamAppID = 0;
        g->debuggerPort = 0;

        GEN8_ParseRoomOrder(&reader, g);
        return 0;
    }

    // WAD8 >= 12
    reader_read_u64_le(&reader, &g->timestamp);
    reader_read_string(&reader, dw, &g->displayName);
    reader_read_u64_le(&reader, &g->activeTargets);
    reader_read_u64_le(&reader, &g->functionClassifications);
    reader_read_i32_le(&reader, &g->steamAppID);

    if (g->wadVersion >= 14) {
        reader_read_u32_le(&reader, &g->debuggerPort);
    } else {
        g->debuggerPort = 0;
    }

    GEN8_ParseRoomOrder(&reader, g);

    // GMS2+ fields
    if (g->major >= 2) {
        reader_skip(&reader, 8);      // firstRandom
        reader_skip(&reader, 8 * 4);  // four random entries

        reader_read_f32_le(&reader, &g->gms2FPS);

        reader_skip(&reader, 4);      // AllowStatistics
        reader_skip(&reader, 16);     // GameGUID
    }

    return 0;
}

void GEN8_Bytedump(DataWin *dw) {
    Chunk chunk = {0};

    assert(find_chunk(dw, "GEN8", &chunk) == 0);
    assert(chunk.offset + chunk.length <= dw->file_size);

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    reader_init(&reader, base, chunk.length);

    printf("GEN8 Bytedump:\n");

    repeat(chunk.length, i) {
        uint8_t byte;
        reader_read_u8(&reader, &byte);

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