#ifndef STRINGS_H
#define STRINGS_H

#include "gmdata.h"

int parse_string_table(DataWin *dw, uint32_t offset, uint32_t length);
const char *resolve_string_ptr(const DataWin *dw, const uint8_t *base, uint32_t offset);
const char *get_string(const DataWin *dw, uint32_t offset);
void string_table_free(StringTable *table);

#endif
