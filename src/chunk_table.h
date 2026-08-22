#ifndef CHUNK_TABLE_H
#define CHUNK_TABLE_H

#include "gmdata.h"

int chunk_table_init(ChunkTable *table);
void chunk_table_free(ChunkTable *table);
int find_chunk(const DataWin *dw, const char *name);
int chunk_exists(const DataWin *dw, const char *name);
int get_chunk(const DataWin *dw, const char *name, Chunk *out);
int parse_form_chunks(DataWin *dw);

#endif
