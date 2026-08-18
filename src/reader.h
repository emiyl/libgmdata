#ifndef READER_H
#define READER_H

#include "types.h"

void Reader_init(Reader *reader, const uint8_t *data, size_t size, size_t offset, const char* name);
size_t Reader_remaining(const Reader *reader);
int Reader_seek(Reader *reader, size_t offset);
int Reader_skip(Reader *reader, int count);
int Reader_readUInt8At(Reader *reader, size_t offset, uint8_t *out);
int Reader_readUInt8(Reader *reader, uint8_t *out);
int Reader_readInt8At(Reader *reader, size_t offset, int8_t *out);
int Reader_readInt8(Reader *reader, int8_t *out);
int Reader_readUInt16At(Reader *reader, size_t offset, uint16_t *out);
int Reader_readUInt16(Reader *reader, uint16_t *out);
int Reader_readInt16At(Reader *reader, size_t offset, int16_t *out);
int Reader_readInt16(Reader *reader, int16_t *out);
int Reader_readBool32At(Reader *reader, size_t offset, bool *out);
int Reader_readBool32(Reader *reader, bool *out);
int Reader_readUInt32At(Reader *reader, size_t offset, uint32_t *out);
int Reader_readUInt32(Reader *reader, uint32_t *out);
int Reader_readInt32At(Reader *reader, size_t offset, int32_t *out);
int Reader_readInt32(Reader *reader, int32_t *out);
int Reader_readFloat32At(Reader *reader, size_t offset, float *out);
int Reader_readFloat32(Reader *reader, float *out);
int Reader_readUInt64At(Reader *reader, size_t offset, uint64_t *out);
int Reader_readUInt64(Reader *reader, uint64_t *out);
int Reader_readInt64At(Reader *reader, size_t offset, int64_t *out);
int Reader_readInt64(Reader *reader, int64_t *out);
int Reader_readBytes(Reader *reader, void *out, size_t len);
int Reader_readString(Reader *reader, DataWin *dw, const char** out);
int Reader_readPointerTable(Reader *reader, uint32_t **out_ptrs, uint32_t *out_count);
const uint8_t *Reader_ptr_at(const Reader *reader, size_t offset);

uint16_t read_u16_le_at(const uint8_t *buffer, size_t size, size_t offset);
uint32_t read_u32_le_at(const uint8_t *buffer, size_t size, size_t offset);

#endif
