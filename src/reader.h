#ifndef READER_H
#define READER_H

#include "types.h"

void reader_init(Reader *reader, const uint8_t *data, size_t size);
size_t reader_remaining(const Reader *reader);
int reader_seek(Reader *reader, size_t offset);
int reader_skip(Reader *reader, size_t count);
int reader_read_u8(Reader *reader, uint8_t *out);
int reader_read_u16_le(Reader *reader, uint16_t *out);
int reader_read_u32_le(Reader *reader, uint32_t *out);
int reader_read_i32_le(Reader *reader, int32_t *out);
int reader_read_f32_le(Reader *reader, float *out);
int reader_read_u64_le(Reader *reader, uint64_t *out);
int reader_read_bytes(Reader *reader, void *out, size_t len);
int reader_read_string(Reader *reader, DataWin *dw, const char** out);
const uint8_t *reader_ptr_at(const Reader *reader, size_t offset);
uint16_t read_u16_le_at(const uint8_t *buffer, size_t size, size_t offset);
uint32_t read_u32_le_at(const uint8_t *buffer, size_t size, size_t offset);

#endif
