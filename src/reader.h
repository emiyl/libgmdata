#ifndef READER_H
#define READER_H

#include "types.h"

void reader_init(Reader *reader, const uint8_t *data, size_t size);
size_t reader_remaining(const Reader *reader);
int reader_seek(Reader *reader, size_t offset);
int reader_skip(Reader *reader, int count);
int reader_read_u8_at(Reader *reader, size_t offset, uint8_t *out);
int reader_read_u8(Reader *reader, uint8_t *out);
int reader_read_i8_at(Reader *reader, size_t offset, int8_t *out);
int reader_read_i8(Reader *reader, int8_t *out);
int reader_read_u16_at(Reader *reader, size_t offset, uint16_t *out);
int reader_read_u16(Reader *reader, uint16_t *out);
int reader_read_i16_at(Reader *reader, size_t offset, int16_t *out);
int reader_read_i16(Reader *reader, int16_t *out);
int reader_read_b32_at(Reader *reader, size_t offset, bool *out);
int reader_read_b32(Reader *reader, bool *out);
int reader_read_u32_at(Reader *reader, size_t offset, uint32_t *out);
int reader_read_u32(Reader *reader, uint32_t *out);
int reader_read_i32_at(Reader *reader, size_t offset, int32_t *out);
int reader_read_i32(Reader *reader, int32_t *out);
int reader_read_f32_at(Reader *reader, size_t offset, float *out);
int reader_read_f32(Reader *reader, float *out);
int reader_read_u64_at(Reader *reader, size_t offset, uint64_t *out);
int reader_read_u64(Reader *reader, uint64_t *out);
int reader_read_i64_at(Reader *reader, size_t offset, int64_t *out);
int reader_read_i64(Reader *reader, int64_t *out);
int reader_read_bytes(Reader *reader, void *out, size_t len);
int reader_read_string(Reader *reader, DataWin *dw, const char** out);
int read_pointer_table(Reader *reader, uint32_t **out_ptrs, uint32_t *out_count);
const uint8_t *reader_ptr_at(const Reader *reader, size_t offset);

uint16_t read_u16_le_at(const uint8_t *buffer, size_t size, size_t offset);
uint32_t read_u32_le_at(const uint8_t *buffer, size_t size, size_t offset);

#endif
