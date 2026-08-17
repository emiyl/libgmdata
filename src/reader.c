#include "reader.h"
#include "strings.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void Reader_init(Reader *reader, const uint8_t *data, size_t size) {
    if (reader == NULL) {
        return;
    }

    reader->data = (uint8_t *)data;
    reader->size = size;
    reader->cursor = 0;
}

size_t Reader_remaining(const Reader *reader) {
    if (reader == NULL || reader->data == NULL) {
        return 0;
    }

    if (reader->cursor > reader->size) {
        return 0;
    }

    return reader->size - reader->cursor;
}

int Reader_seek(Reader *reader, size_t offset) {
    if (reader == NULL || reader->data == NULL) {
        return -1;
    }

    if (offset > reader->size) {
        return -1;
    }

    reader->cursor = offset;
    return 0;
}

int Reader_skip(Reader* reader, int count) {
    int cursor = reader->cursor;
    int new_pos = cursor + count;
    return Reader_seek(reader, new_pos);
}

static int _read_at_check(const Reader *reader, size_t offset, size_t size) {
    if (reader == NULL || reader->data == NULL) {
        return -1;
    }

    if (offset > reader->size || size > reader->size - offset) {
        return -1;
    }

    return 0;
}

int _read_u8_at(Reader *reader, size_t offset, uint8_t *out) {
    if (out == NULL || _read_at_check(reader, offset, sizeof(uint8_t)) != 0) {
        return -1;
    }

    *out = reader->data[offset];
    return 0;
}

int _read_u16_at(Reader *reader, size_t offset, uint16_t *out) {
    if (out == NULL || _read_at_check(reader, offset, sizeof(uint16_t)) != 0) {
        return -1;
    }

    *out = (uint16_t)reader->data[offset]
         | ((uint16_t)reader->data[offset + 1] << 8);

    return 0;
}

int _read_u32_at(Reader *reader, size_t offset, uint32_t *out) {
    if (out == NULL || _read_at_check(reader, offset, sizeof(uint32_t)) != 0) {
        return -1;
    }

    *out = (uint32_t)reader->data[offset]
         | ((uint32_t)reader->data[offset + 1] << 8)
         | ((uint32_t)reader->data[offset + 2] << 16)
         | ((uint32_t)reader->data[offset + 3] << 24);

    return 0;
}

int _read_u64_at(Reader *reader, size_t offset, uint64_t *out) {
    if (out == NULL || _read_at_check(reader, offset, sizeof(uint64_t)) != 0) {
        return -1;
    }

    *out = (uint64_t)reader->data[offset]
         | ((uint64_t)reader->data[offset + 1] << 8)
         | ((uint64_t)reader->data[offset + 2] << 16)
         | ((uint64_t)reader->data[offset + 3] << 24)
         | ((uint64_t)reader->data[offset + 4] << 32)
         | ((uint64_t)reader->data[offset + 5] << 40)
         | ((uint64_t)reader->data[offset + 6] << 48)
         | ((uint64_t)reader->data[offset + 7] << 56);

    return 0;
}

int Reader_read_u8_at(Reader *reader, size_t offset, uint8_t *out) {
    return _read_u8_at(reader, offset, out);
}

int Reader_read_u8(Reader *reader, uint8_t *out) {
    return Reader_read_u8_at(reader, reader->cursor, out);
}

int Reader_read_i8_at(Reader *reader, size_t offset, int8_t *out) {
    uint8_t value;
    if (_read_u8_at(reader, offset, &value) != 0) {
        return -1;
    }

    *out = (int8_t)value;
    return 0;
}

int Reader_read_i8(Reader *reader, int8_t *out) {
    return Reader_read_i8_at(reader, reader->cursor, out);
}

int Reader_read_u16_at(Reader *reader, size_t offset, uint16_t *out) {
    return _read_u16_at(reader, offset, out);
}

int Reader_read_u16(Reader *reader, uint16_t *out) {
    return Reader_read_u16_at(reader, reader->cursor, out);
}

int Reader_read_i16_at(Reader *reader, size_t offset, int16_t *out) {
    uint16_t value;
    if (_read_u16_at(reader, offset, &value) != 0) {
        return -1;
    }

    *out = (int16_t)value;
    return 0;
}

int Reader_read_i16(Reader *reader, int16_t *out) {
    return Reader_read_i16_at(reader, reader->cursor, out);
}

int Reader_read_b32_at(Reader *reader, size_t offset, bool *out) {
    uint32_t value;
    if (_read_u32_at(reader, offset, &value) != 0) {
        return -1;
    }

    *out = value != 0;
    return 0;
}

int Reader_read_b32(Reader *reader, bool *out) {
    return Reader_read_b32_at(reader, reader->cursor, out);
}

int Reader_read_u32_at(Reader *reader, size_t offset, uint32_t *out) {
    return _read_u32_at(reader, offset, out);
}

int Reader_read_u32(Reader *reader, uint32_t *out) {
    return Reader_read_u32_at(reader, reader->cursor, out);
}

int Reader_read_i32_at(Reader *reader, size_t offset, int32_t *out) {
    uint32_t value;
    if (_read_u32_at(reader, offset, &value) != 0) {
        return -1;
    }

    *out = (int32_t)value;

    return 0;
}

int Reader_read_i32(Reader *reader, int32_t *out) {
    return Reader_read_i32_at(reader, reader->cursor, out);
}

int Reader_read_f32_at(Reader *reader, size_t offset, float *out) {
    uint32_t value;
    if (_read_u32_at(reader, offset, &value) != 0) {
        return -1;
    }

    memcpy(out, &value, sizeof(float));

    return 0;
}

int Reader_read_f32(Reader *reader, float *out) {
    return Reader_read_f32_at(reader, reader->cursor, out);
}

int Reader_read_u64_at(Reader *reader, size_t offset, uint64_t *out) {
    return _read_u64_at(reader, offset, out);
}

int Reader_read_u64(Reader *reader, uint64_t *out) {
    return Reader_read_u64_at(reader, reader->cursor, out);
}

int Reader_read_i64_at(Reader *reader, size_t offset, int64_t *out) {
    uint64_t value;
    if (_read_u64_at(reader, offset, &value) != 0) {
        return -1;
    }

    *out = (int64_t)value;

    return 0;
}

int Reader_read_i64(Reader *reader, int64_t *out) {
    return Reader_read_i64_at(reader, reader->cursor, out);
}

int Reader_read_bytes(Reader *reader, void *out, size_t len) {
    if (reader == NULL || reader->data == NULL || out == NULL) {
        return -1;
    }

    if (reader->cursor + len > reader->size) {
        return -1;
    }

    for (size_t i = 0; i < len; ++i) {
        uint8_t byte = reader->data[reader->cursor + i];
        ((uint8_t *)out)[i] = byte;
    }

    reader->cursor += len;
    return 0;
}

int Reader_read_string(Reader *reader, DataWin *dw, const char **out) {
    uint32_t offset;

    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    if (Reader_read_u32(reader, &offset) != 0) {
        return -1;
    }

    *out = get_string(dw, offset);

    return *out != NULL ? 0 : -1;
}

int read_pointer_table(Reader *reader, uint32_t **out_ptrs, uint32_t *out_count) {
    if (reader == NULL || out_ptrs == NULL || out_count == NULL) {
        return -1;
    }

    uint32_t count;
    if (Reader_read_u32(reader, &count) != 0) {
        return -1;
    }

    if (count == 0) {
        *out_ptrs = NULL;
        *out_count = 0;
        return 0;
    }

    uint32_t *ptrs = (uint32_t *)malloc(count * sizeof(uint32_t));
    if (ptrs == NULL) {
        return -1;
    }

    for (uint32_t i = 0; i < count; ++i) {
        if (Reader_read_u32(reader, &ptrs[i]) != 0) {
            free(ptrs);
            return -1;
        }
    }

    *out_ptrs = ptrs;
    *out_count = count;

    return 0;
}

const uint8_t *Reader_ptr_at(const Reader *reader, size_t offset) {
    if (reader == NULL || reader->data == NULL || offset > reader->size) {
        return NULL;
    }

    return reader->data + offset;
}

uint16_t read_u16_le_at(const uint8_t *buffer, size_t size, size_t offset) {
    if (buffer == NULL || offset + sizeof(uint16_t) > size) {
        return 0;
    }

    return (uint16_t)buffer[offset]
         | ((uint16_t)buffer[offset + 1] << 8);
}

uint32_t read_u32_le_at(const uint8_t *buffer, size_t size, size_t offset) {
    if (buffer == NULL || offset + sizeof(uint32_t) > size) {
        return 0;
    }

    return (uint32_t)buffer[offset]
         | ((uint32_t)buffer[offset + 1] << 8)
         | ((uint32_t)buffer[offset + 2] << 16)
         | ((uint32_t)buffer[offset + 3] << 24);
}