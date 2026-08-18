#ifndef CHUNKS_H
#define CHUNKS_H

#include "../types.h"

int GEN8_parse(DataWin *dw);
void GEN8_free(Gen8 *g);

int OPTN_parse(DataWin *dw);
void OPTN_free(Optn *o);

int LANG_parse(DataWin *dw);
void LANG_free(Lang *l);

int EXTN_parse(DataWin *dw);
void EXTN_free(Extn *e);

int SOND_parse(DataWin *dw);
void SOND_free(Sond *s);

int AGRP_parse(DataWin *dw);
void AGRP_free(Agrp *a);

int SPRT_parse(DataWin *dw);
void SPRT_free(Sprt *s); 

int BGND_parse(DataWin *dw);
void BGND_free(Bgnd *b);

int PATH_parse(DataWin *dw);
void PATH_free(Path *p);

int SCPT_parse(DataWin *dw);
void SCPT_free(Scpt *s);

#endif