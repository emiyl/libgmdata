#ifndef DATAWIN_H
#define DATAWIN_H

#include "gmdata.h"

typedef enum {
    DATAWINLOADTYPE_LOAD_PER_CHUNK = 0,
    DATAWINLOADTYPE_LOAD_IN_MEMORY_AHEAD_OF_TIME = 1,
    DATAWINLOADTYPE_MAP_FILE = 2
} DataWinLoadType;

typedef struct {
    const char *string;
    bool value;
} StringBooleanEntry;

typedef struct {
    bool parseGen8;
    bool parseOptn;
    bool parseLang;
    bool parseExtn;
    bool parseSond;
    bool parseAgrp;
    bool parseSprt;
    bool parseBgnd;
    bool parsePath;
    bool parseScpt;
    bool parseGlob;
    bool parseShdr;
    bool parseFont;
    bool parseTmln;
    bool parseObjt;
    bool parseRoom;
    bool parseTpag;
    bool parseCode;
    bool parseVari;
    bool parseFunc;
    bool parseStrg;
    bool parseTxtr;
    bool parseAudo;
    bool skipLoadingPreciseMasksForNonPreciseSprites;
    bool lazyLoadRooms;
    bool lazyLoadTextures;
    bool lazyLoadAudio;
    StringBooleanEntry *eagerlyLoadedRooms;
    DataWinLoadType loadType;
    void (*progressCallback)(const char *chunkName, int chunkIndex, int totalChunks, DataWin *dataWin, void *userData);
    void *progressCallbackUserData;
} DataWinParserOptions;

void DataWin_initParserOptions(DataWinParserOptions *options);
void DataWin_applyParserOptions(DataWin *dw, const DataWinParserOptions *options);
int DataWin_parseWithOptions(DataWin *dw, const DataWinParserOptions *options);
bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);
void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);

#endif
