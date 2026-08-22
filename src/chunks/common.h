#ifndef CHUNKS_COMMON_H
#define CHUNKS_COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../utils.h"
#include "../gmdata.h"
#include "../chunk_table.h"
#include "../reader.h"

#define read(out, type) \
    if (Reader_read##type(reader, out) != 0) { \
        logWarn("[read] Failed to read " #type " at offset %zu\n", reader->cursor); \
        return -1; \
    }

#define readString(out, dw) \
    if (Reader_readString(reader, dw, out) != 0) { \
        logWarn("[readString] Failed to read string at offset %zu\n", reader->cursor); \
        return -1; \
    }\

#endif