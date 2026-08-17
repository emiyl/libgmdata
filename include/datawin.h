#ifndef DATAWIN_H
#define DATAWIN_H

#include "types.h"

int load_file(DataWin *dw, const char *path);
int parse(DataWin *dw);
void datawin_free(DataWin *dw);

#endif
