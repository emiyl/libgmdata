#ifndef CHUNKS_H
#define CHUNKS_H

#include "types.h"

int chunk_table_init(ChunkTable *table);
void chunk_table_free(ChunkTable *table);
int find_chunk(const DataWin *dw, const char *name, Chunk *out);
int parse_form_chunks(DataWin *dw);

#endif
