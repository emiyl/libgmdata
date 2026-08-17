#ifndef GEN8_H
#define GEN8_H

#include "../types.h"

Gen8 GEN8_Parse(DataWin *dw);
void GEN8_Print(Gen8 gen8);
void GEN8_Bytedump(DataWin *dw);

#endif