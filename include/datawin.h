#ifndef DATAWIN_H
#define DATAWIN_H

#include "types.h"

int DataWin_loadFile(DataWin *dw, const char *path);
int DataWin_parse(DataWin *dw);
void DataWin_free(DataWin *dw);

bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);
void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);

#endif
