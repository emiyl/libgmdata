#ifndef DATAWIN_H
#define DATAWIN_H

#include "gmdata.h"

bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);
void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);

#endif
