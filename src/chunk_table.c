#include "chunk_table.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int chunk_table_init(ChunkTable *table) {
    if (table == NULL) {
        return -1;
    }

    table->items = NULL;
    table->count = 0;
    table->capacity = 0;
    return 0;
}

void chunk_table_free(ChunkTable *table) {
    if (table == NULL) {
        return;
    }

    free(table->items);
    table->items = NULL;
    table->count = 0;
    table->capacity = 0;
}

int find_chunk(const DataWin *dw, const char *name) {
    if (dw == NULL || name == NULL) {
        return -1;
    }

    for (size_t i = 0; i < dw->chunks.count; ++i) {
        if (memcmp(dw->chunks.items[i].name, name, 4) == 0) {
            return i;
        }
    }

    return -1;
}

int chunk_exists(const DataWin *dw, const char *name) {
    return find_chunk(dw, name) >= 0;
}

int get_chunk(const DataWin *dw, const char *name, Chunk *out) {
    int chunk_pos = find_chunk(dw, name);
    if (chunk_pos < 0) return -1;

    *out = dw->chunks.items[chunk_pos];
    return 0;
}

int parse_form_chunks(DataWin *dw) {
    if (dw == NULL || dw->file_data == NULL || dw->file_size < 12U) {
        return -1;
    }

    if (memcmp(dw->file_data, "FORM", 4) != 0) {
        return -1;
    }

    uint32_t form_size = (uint32_t)dw->file_data[4]
                       | ((uint32_t)dw->file_data[5] << 8)
                       | ((uint32_t)dw->file_data[6] << 16)
                       | ((uint32_t)dw->file_data[7] << 24);

    size_t offset = 8;
    size_t end = 8U + (size_t)form_size;
    if (end > dw->file_size) {
        end = dw->file_size;
    }

    chunk_table_free(&dw->chunks);
    dw->chunks.items = NULL;
    dw->chunks.count = 0;
    dw->chunks.capacity = 0;

    while (offset + 8U <= end) {
        char name[5] = {0};
        memcpy(name, dw->file_data + offset, 4);

        uint32_t length = (uint32_t)dw->file_data[offset + 4]
                        | ((uint32_t)dw->file_data[offset + 5] << 8)
                        | ((uint32_t)dw->file_data[offset + 6] << 16)
                        | ((uint32_t)dw->file_data[offset + 7] << 24);

        if (offset + 8U + length > end) {
            break;
        }

        Chunk chunk;
        memcpy(chunk.name, name, 4);
        chunk.name[4] = '\0';
        chunk.offset = (uint32_t)(offset + 8U);
        chunk.length = length;

        if (dw->chunks.count == dw->chunks.capacity) {
            size_t new_capacity = dw->chunks.capacity == 0 ? 8U : dw->chunks.capacity * 2U;
            Chunk *new_items = (Chunk *)realloc(dw->chunks.items, new_capacity * sizeof(Chunk));
            if (new_items == NULL) {
                return -1;
            }
            dw->chunks.items = new_items;
            dw->chunks.capacity = new_capacity;
        }

        dw->chunks.items[dw->chunks.count++] = chunk;
        offset += 8U + length;
    }

    return 0;
}
