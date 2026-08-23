#ifndef CHUNKS_H
#define CHUNKS_H

#include "../gmdata.h"

int GEN8_parse(DataWin *dw);
int GEN8_free(Gen8Chunk *g);

int OPTN_parse(DataWin *dw);
int OPTN_free(OptnChunk *o);

int LANG_parse(DataWin *dw);
int LANG_free(LangChunk *l);

int EXTN_parse(DataWin *dw);
int EXTN_free(ExtnChunk *e);

int SOND_parse(DataWin *dw);
int SOND_free(SondChunk *s);

int AGRP_parse(DataWin *dw);
int AGRP_free(AgrpChunk *a);

int SPRT_parse(DataWin *dw);
int SPRT_free(SprtChunk *s); 

int BGND_parse(DataWin *dw);
int BGND_free(BgndChunk *b);

int TPAG_parse(DataWin *dw);
int TPAG_free(TpagChunk *t);

int PATH_parse(DataWin *dw);
int PATH_free(PathChunk *p);

int SCPT_parse(DataWin *dw);
int SCPT_free(ScptChunk *s);

int GLOB_parse(DataWin *dw);
int GLOB_free(GlobChunk *g);

int CODE_parse(DataWin *dw);
int CODE_free(CodeChunk *c);

int VARI_parse(DataWin *dw);
int VARI_free(VariChunk *v);

int FUNC_parse(DataWin *dw);
int FUNC_free(FuncChunk *f);

int SHDR_parse(DataWin *dw);
int SHDR_free(ShdrChunk *s);

int FONT_parse(DataWin *dw);
int FONT_free(FontChunk *f);

int TMLN_parse(DataWin *dw);
int TMLN_free(TmlnChunk *t);

int OBJT_parse(DataWin *dw);
int OBJT_free(ObjtChunk *o);

int ROOM_parse(DataWin *dw);
int ROOM_free(RoomChunk *r);

int STRG_parse(DataWin *dw);
int STRG_free(StrgChunk *s);

int TGIN_parse(DataWin *dw);
int TGIN_free(TginChunk *t);

int ACRV_parse(DataWin *dw);
int ACRV_free(AcrvChunk *a);

int TXTR_parse(DataWin *dw);
int TXTR_free(TxtrChunk *t);

int AUDO_parse(DataWin *dw);
int AUDO_free(AudoChunk *a);

#endif