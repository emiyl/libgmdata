#ifndef DATAWIN_H
#define DATAWIN_H

#include "types.h"

int DataWin_loadFile(DataWin *dw, const char *path);
int DataWin_parse(DataWin *dw);
void DataWin_free(DataWin *dw);

#endif
