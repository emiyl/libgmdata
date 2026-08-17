#include "reader.h"
#include "strings.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void reader_init(Reader *reader, const uint8_t *data, size_t size) {
    if (reader == NULL) {
        return;
    }

    reader->data = (uint8_t *)data;
    reader->size = size;
    reader->cursor = 0;
}

size_t reader_remaining(const Reader *reader) {
    if (reader == NULL || reader->data == NULL) {
        return 0;
    }

    if (reader->cursor > reader->size) {
        return 0;
    }

    return reader->size - reader->cursor;
}

int reader_seek(Reader *reader, size_t offset) {
    if (reader == NULL || reader->data == NULL) {
        return -1;
    }

    if (offset > reader->size) {
        return -1;
    }

    reader->cursor = offset;
    return 0;
}

int reader_skip(Reader* reader, int count) {
    int cursor = reader->cursor;
    int new_pos = cursor + count;
    return reader_seek(reader, new_pos);
}

int reader_read_u8(Reader *reader, uint8_t *out) {
    if (reader == NULL || reader->data == NULL || out == NULL) {
        return -1;
    }

    if (reader->cursor + sizeof(uint8_t) > reader->size) {
        return -1;
    }

    uint8_t value = reader->data[reader->cursor];

    *out = value;
    reader->cursor += sizeof(uint8_t);

    return 0;
}

int reader_read_u16_le(Reader *reader, uint16_t *out) {
    if (reader == NULL || reader->data == NULL || out == NULL) {
        return -1;
    }

    if (reader->cursor + sizeof(uint16_t) > reader->size) {
        return -1;
    }

    uint16_t value = (uint16_t)reader->data[reader->cursor]
         | ((uint16_t)reader->data[reader->cursor + 1] << 8);

    *out = value;
    reader->cursor += sizeof(uint16_t);

    return 0;
}

int _read_u32(Reader *reader, uint32_t *out) {
    if (reader == NULL || reader->data == NULL || out == NULL) {
        return -1;
    }

    if (reader->cursor + sizeof(uint32_t) > reader->size) {
        return -1;
    }

    uint32_t value = (uint32_t)reader->data[reader->cursor]
         | ((uint32_t)reader->data[reader->cursor + 1] << 8)
         | ((uint32_t)reader->data[reader->cursor + 2] << 16)
         | ((uint32_t)reader->data[reader->cursor + 3] << 24);

    *out = value;
    reader->cursor += sizeof(uint32_t);

    return 0;
}

int reader_read_b32_le(Reader *reader, bool *out) {
    uint32_t value;
    if (_read_u32(reader, &value) != 0) {
        return -1;
    }

    *out = value != 0;
    return 0;
}

int reader_read_u32_le(Reader *reader, uint32_t *out) {
    uint32_t value;
    if (_read_u32(reader, &value) != 0) {
        return -1;
    }

    *out = value;
    
    return 0;
}

int reader_read_i32_le(Reader *reader, int32_t *out) {
    uint32_t value;
    if (_read_u32(reader, &value) != 0) {
        return -1;
    }

    *out = (int32_t)value;

    return 0;
}

int reader_read_f32_le(Reader *reader, float *out) {
    uint32_t value;
    if (_read_u32(reader, &value) != 0) {
        return -1;
    }

    memcpy(out, &value, sizeof(float));

    return 0;
}

int reader_read_u64_le(Reader *reader, uint64_t *out) {
    if (reader == NULL || reader->data == NULL || out == NULL) {
        return -1;
    }

    if (reader->cursor + sizeof(uint64_t) > reader->size) {
        return -1;
    }

    uint64_t value = (uint64_t)reader->data[reader->cursor]
         | ((uint64_t)reader->data[reader->cursor + 1] << 8)
         | ((uint64_t)reader->data[reader->cursor + 2] << 16)
         | ((uint64_t)reader->data[reader->cursor + 3] << 24)
         | ((uint64_t)reader->data[reader->cursor + 4] << 32)
         | ((uint64_t)reader->data[reader->cursor + 5] << 40)
         | ((uint64_t)reader->data[reader->cursor + 6] << 48)
         | ((uint64_t)reader->data[reader->cursor + 7] << 56);

    *out = value;
    reader->cursor += sizeof(uint64_t);

    return 0;
}

int reader_read_bytes(Reader *reader, void *out, size_t len) {
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

int reader_read_string(Reader *reader, DataWin *dw, const char **out) {
    uint32_t offset;

    if (reader == NULL || dw == NULL || out == NULL) {
        return -1;
    }

    if (_read_u32(reader, &offset) != 0) {
        return -1;
    }

    *out = get_string(dw, offset);

    return *out != NULL ? 0 : -1;
}

const uint8_t *reader_ptr_at(const Reader *reader, size_t offset) {
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