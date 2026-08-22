#include "strings.h"

#include "reader.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char *dup_string_region(const uint8_t *buffer, size_t size, size_t offset, size_t length) {
    if (buffer == NULL || offset + length > size) {
        return NULL;
    }

    char *copy = (char *)malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, buffer + offset, length);
    copy[length] = '\0';
    return copy;
}

int parse_string_table(DataWin *dw, uint32_t offset, uint32_t length) {
    if (dw == NULL || dw->file_data == NULL || offset + length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + offset;
    size_t remaining = length;
    if (remaining < sizeof(uint32_t)) {
        return -1;
    }

    uint32_t count = read_u32_le_at(base, remaining, 0);
    size_t table_size = sizeof(uint32_t) + (size_t)count * sizeof(uint32_t);
    if (table_size > remaining) {
        return -1;
    }

    string_table_free(&dw->strings);
    dw->strings.entries = (StringEntry *)calloc((size_t)count, sizeof(StringEntry));
    if (dw->strings.entries == NULL && count > 0U) {
        return -1;
    }

    dw->strings.count = count; 
    dw->strings.capacity = count;

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t string_offset = read_u32_le_at(base, remaining, sizeof(uint32_t) + i * sizeof(uint32_t));
        dw->strings.entries[i].offset = string_offset;
        dw->strings.entries[i].text = NULL;

        if (string_offset == 0U) {
            continue;
        }

        if (string_offset + sizeof(uint32_t) > dw->file_size) {
            continue;
        }

        uint32_t len = read_u32_le_at(dw->file_data, dw->file_size, string_offset);
        if (string_offset + sizeof(uint32_t) + len > dw->file_size) {
            continue;
        }

        dw->strings.entries[i].text = dup_string_region(dw->file_data, dw->file_size, string_offset + sizeof(uint32_t), len);
    }

    return 0;
}

const char *resolve_string_ptr(const DataWin *dw, const uint8_t *base, uint32_t offset) {
    (void)base;
    if (dw == NULL || dw->file_data == NULL || offset == 0U) {
        return NULL;
    }

    uint32_t prefix_offset = offset;
    if (prefix_offset >= 4U && prefix_offset <= dw->file_size) {
        uint32_t maybe_length = read_u32_le_at(dw->file_data, dw->file_size, prefix_offset - 4U);
        if (prefix_offset >= 4U && prefix_offset - 4U + sizeof(uint32_t) <= dw->file_size && maybe_length <= dw->file_size - (prefix_offset - 4U) - sizeof(uint32_t)) {
            return (const char *)(dw->file_data + (prefix_offset - 4U) + sizeof(uint32_t));
        }
    }

    if (offset + sizeof(uint32_t) > dw->file_size) {
        return NULL;
    }

    uint32_t length = read_u32_le_at(dw->file_data, dw->file_size, offset);
    if (offset + sizeof(uint32_t) + length > dw->file_size) {
        return NULL;
    }

    static char empty_string[] = "";
    if (length == 0U) {
        return empty_string;
    }

    return (const char *)(dw->file_data + offset + sizeof(uint32_t));
}

const char *get_string(const DataWin *dw, uint32_t offset) {
    if (dw == NULL || offset == 0U) {
        return NULL;
    }

    for (size_t i = 0; i < dw->strings.count; ++i) {
        if (dw->strings.entries[i].offset == offset) {
            return dw->strings.entries[i].text;
        }

        if (dw->strings.entries[i].offset + 4U == offset) {
            return dw->strings.entries[i].text;
        }
    }

    return NULL;
}

int string_table_free(StringTable *table) {
    if (table == NULL) {
        return -1;
    }

    if (table->entries != NULL) {
        for (size_t i = 0; i < table->count; ++i) {
            free(table->entries[i].text);
        }
        free(table->entries);
        table->entries = NULL;
    }

    table->count = 0;
    table->capacity = 0;
    return 0;
}
