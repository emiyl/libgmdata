#ifndef GEN8_H
#define GEN8_H

#include "../types.h"

Gen8 Gen8_Parse(DataWin *dw);
void Gen8_Print(Gen8 gen8);
void Gen8_Bytedump(DataWin *dw);

#endif