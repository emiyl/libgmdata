#ifndef CHUNKS_H
#define CHUNKS_H

#include "../types.h"

int GEN8_parse(DataWin *dw);
void GEN8_free(Gen8Chunk *g);

int OPTN_parse(DataWin *dw);
void OPTN_free(OptnChunk *o);

int LANG_parse(DataWin *dw);
void LANG_free(LangChunk *l);

int EXTN_parse(DataWin *dw);
void EXTN_free(ExtnChunk *e);

int SOND_parse(DataWin *dw);
void SOND_free(SondChunk *s);

int AGRP_parse(DataWin *dw);
void AGRP_free(AgrpChunk *a);

int SPRT_parse(DataWin *dw);
void SPRT_free(SprtChunk *s); 

int BGND_parse(DataWin *dw);
void BGND_free(BgndChunk *b);

int PATH_parse(DataWin *dw);
void PATH_free(PathChunk *p);

int SCPT_parse(DataWin *dw);
void SCPT_free(ScptChunk *s);

int GLOB_parse(DataWin *dw);
void GLOB_free(GlobChunk *g);

int SHDR_parse(DataWin *dw);
void SHDR_free(ShdrChunk *s);

int FONT_parse(DataWin *dw);
void FONT_free(FontChunk *f);

#endif