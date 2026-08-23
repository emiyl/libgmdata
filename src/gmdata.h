#ifndef GMDATA_H
#define GMDATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t size;
    size_t offset;
    size_t cursor;
    const char* name; // Optional: Name of the data source for logging/debugging
} Reader;

typedef struct {
    char *text;
    uint32_t offset;
} StringEntry;

typedef struct {
    StringEntry *entries;
    size_t count;
    size_t capacity;
} StringTable;

typedef struct {
    char name[5];
    uint32_t offset;
    uint32_t length;
} Chunk;

typedef struct {
    Chunk *items;
    size_t count;
    size_t capacity;
} ChunkTable;

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t release;
    uint32_t build;
} DetectedFormat;

typedef struct {
    uint8_t isDebuggerDisabled;
    uint8_t wadVersion;
    const char* fileName;
    const char* config;
    uint32_t lastObj;
    uint32_t lastTile;
    uint32_t gameID;
    uint8_t directPlayGuid[16];
    const char* name;
    uint32_t major;
    uint32_t minor;
    uint32_t release;
    uint32_t build;
    uint32_t defaultWindowWidth;
    uint32_t defaultWindowHeight;
    uint32_t info;
    uint32_t licenseCRC32;
    uint8_t licenseMD5[16];
    uint64_t timestamp;
    const char* displayName;
    uint64_t activeTargets;
    uint64_t functionClassifications;
    int32_t steamAppID;
    uint32_t debuggerPort;
    uint32_t roomOrderCount;
    int32_t* roomOrder;
    float gms2FPS;
} Gen8Chunk;



typedef struct {
    const char* name;
    const char* value;
} OptnConstant;

#define INFO_FULLSCREEN         (1u << 0)
#define INFO_INTERPOLATE_PIXELS (1u << 1)
#define INFO_USE_NEW_AUDIO      (1u << 2)
#define INFO_NO_BORDER          (1u << 3)
#define INFO_SHOW_CURSOR        (1u << 4)
#define INFO_SIZABLE            (1u << 5)
#define INFO_STAY_ON_TOP        (1u << 6)
#define INFO_CHANGE_RESOLUTION  (1u << 7)
#define INFO_NO_BUTTONS         (1u << 8)
#define INFO_SCREEN_KEY         (1u << 9)
#define INFO_HELP_KEY           (1u << 10)
#define INFO_QUIT_KEY           (1u << 11)
#define INFO_SAVE_KEY           (1u << 12)
#define INFO_SCREENSHOT_KEY     (1u << 13)
#define INFO_CLOSE_SEC          (1u << 14)
#define INFO_FREEZE             (1u << 15)
#define INFO_SHOW_PROGRESS      (1u << 16)
#define INFO_LOAD_TRANSPARENT   (1u << 17)
#define INFO_SCALE_PROGRESS     (1u << 18)
#define INFO_DISPLAY_ERRORS     (1u << 19)
#define INFO_WRITE_ERRORS       (1u << 20)
#define INFO_ABORT_ERRORS       (1u << 21)
#define INFO_VARIABLE_ERRORS    (1u << 22)
#define INFO_CREATION_EVENT_ORDER (1u << 23)

typedef struct {
    int32_t shaderExtensionFlag;
    int32_t shaderExtensionVersion;
    uint64_t info;
    int32_t scale;
    uint32_t windowColor;
    uint32_t colorDepth;
    uint32_t resolution;
    uint32_t frequency;
    uint32_t vertexSync;
    uint32_t priority;
    uint32_t backImage;
    uint32_t frontImage;
    uint32_t loadImage;
    uint32_t loadAlpha;
    uint32_t constantCount;
    OptnConstant* constants;
} OptnChunk;

typedef struct {
    const char* name;
    const char* region;
    uint32_t entryCount;
    const char** entries;
} Language;

typedef struct {
    uint32_t unknown1;
    uint32_t languageCount;
    uint32_t entryCount;
    const char** entryIds;
    Language* languages;
} LangChunk;

typedef struct {
    const char* name;
    uint32_t id;
    uint32_t kind;
    uint32_t retType;
    const char* extName;
    uint32_t argumentCount;
    uint32_t* arguments;
} ExtensionFunction;

typedef struct {
    const char* filename;
    const char* cleanupScript;
    const char* initScript;
    uint32_t kind;
    uint32_t functionCount;
    ExtensionFunction* functions;
} ExtensionFile;

typedef struct {
    const char* folderName;
    const char* name;
    const char* className;
    uint32_t fileCount;
    ExtensionFile* files;
} Extension;

typedef struct {
    uint32_t count;
    Extension* extensions;
} ExtnChunk;

typedef enum {
    AUDIO_ENTRY_FLAG_IS_EMBEDDED = 0x01,
    AUDIO_ENTRY_FLAG_IS_COMPRESSED = 0x02,
    AUDIO_ENTRY_FLAG_IS_DECOMPRESSED_ON_LOAD = 0x03,
    AUDIO_ENTRY_FLAG_REGULAR = 0x64
} AudioEntryFlags;

typedef struct {
    bool present;
    const char* name;
    uint32_t flags;
    const char* type;
    const char* file;
    uint32_t effects;
    float volume;
    float pitch;
    float pan; // -1.0 = full left, 0.0 = center, +1.0 = full right. Legacy field that is not used in WAD11+.
    int32_t audioGroup;
    int32_t audioFile;
} Sound;

typedef struct {
    uint32_t count;
    Sound* sounds;
} SondChunk;

typedef struct {
    bool present;
    const char* name;
    const char* path; // nullptr for pre-GM 2024.14+ games
} AudioGroup;

typedef struct {
    uint32_t count;
    AudioGroup* audioGroups;
} AgrpChunk;

typedef struct {
    bool present;
    const char* name;
    uint32_t width;
    uint32_t height;
    int32_t marginLeft;
    int32_t marginRight;
    int32_t marginBottom;
    int32_t marginTop;
    bool transparent;
    bool smooth;
    bool preload;
    uint32_t bboxMode;
    uint32_t sepMasks;
    int32_t originX;
    int32_t originY;
    uint32_t sVersion;
    uint32_t sSpriteType;
    float gms2PlaybackSpeed;
    bool gms2PlaybackSpeedType;
    bool specialType;
    uint32_t textureCount;
    int32_t* tpagIndices;    // resolved TPAG indices (one per frame); -1 for unresolved
    uint32_t maskCount;       // number of collision masks (one per frame, or 0)
    uint8_t** masks;          // array of maskCount packed bit arrays (nullptr if none)
    bool maskDataOwned;       // true when masks[] entries were heap-allocated; false when they point into the file mapping
    // Collision mask storage dimensions. Pre-2024.6 these equal the full sprite width/height with zero offset.
    // GMS 2024.6+ stores masks at bounding-box dimensions, so the mask covers only [maskOffsetX, maskOffsetX+maskWidth).
    uint32_t maskWidth;
    uint32_t maskHeight;
    int32_t maskOffsetX;      // sprite-local X of the mask's left edge (marginLeft on 2024.6+, else 0)
    int32_t maskOffsetY;      // sprite-local Y of the mask's top edge (marginTop on 2024.6+, else 0)
    // Nine-slice (GMS2 sVersion >= 3). Present iff the sprite stored a non-zero nineSliceOffset.
    bool nineSliceEnabled;
    int32_t nsLeft;
    int32_t nsTop;
    int32_t nsRight;
    int32_t nsBottom;
    uint8_t nsTileModes[5];   // order: Left, Top, Right, Bottom, Center. 0=Stretch, 1=Repeat, 2=Mirror, 3=BlankRepeat, 4=Hide
} Sprite;

typedef struct {
    uint32_t count;
    uint32_t parsedCount; // number of sprites loaded from SPRT; slots >= parsedCount are runtime-allocated and own their `name`
    Sprite* sprites;
} SprtChunk;

typedef struct {
    bool present;
    const char* name;
    bool transparent;
    bool smooth;
    bool preload;
    int32_t tpagIndex;      // resolved TPAG index, -1 if unresolved
    uint32_t gms2UnknownAlways2;
    uint32_t gms2TileWidth;
    uint32_t gms2TileHeight;
    uint32_t gms2TileSeparationX;
    uint32_t gms2TileSeparationY;
    uint32_t gms2OutputBorderX;
    uint32_t gms2OutputBorderY;
    uint32_t gms2TileColumns;
    uint32_t gms2ItemsPerTileCount;
    uint32_t gms2TileCount;
    int gms2ExportedSpriteIndex;
    int64_t gms2FrameLength;
    uint32_t *gms2TileIds;
} Background;

typedef struct {
    uint32_t count;
    Background* backgrounds;
} BgndChunk;

typedef struct {
    float x;
    float y;
    float speed;
} PathPoint;

typedef struct {
    float x;
    float y;
    float speed;
    float l; // cumulative arc length from start
} InternalPathPoint;

typedef struct {
    float x;
    float y;
    float speed;
} PathPositionResult;

typedef struct {
    bool present;
    const char* name;
    bool isSmooth;
    bool isClosed;
    uint32_t precision;
    uint32_t pointCount;
    PathPoint* points;
    uint32_t internalPointCount;
    InternalPathPoint* internalPoints;
    float length; // total arc length
    bool exists;
} GamePath;

typedef struct {
    uint32_t count;
    GamePath* paths;
} PathChunk;

typedef struct {
    bool present;
    const char* name;
    int32_t codeId;
} Script;

typedef struct {
    uint32_t count;
    Script* scripts;
} ScptChunk;

typedef struct {
    uint32_t count;
    int32_t* codeIds;
} GlobChunk;

typedef struct {
    bool present;
    const char* name;
    uint32_t type;
    const char* glslES_Vertex;
    const char* glslES_Fragment;
    const char* glsl_Vertex;
    const char* glsl_Fragment;
    const char* hlsl9_Vertex;
    const char* hlsl9_Fragment;
    uint32_t hlsl11_VertexOffset;
    uint32_t hlsl11_PixelOffset;
    uint32_t vertexAttributeCount;
    const char** vertexAttributes;
    int32_t version;
    uint32_t pssl_VertexOffset;
    uint32_t pssl_VertexLen;
    uint32_t pssl_PixelOffset;
    uint32_t pssl_PixelLen;
    uint32_t cgVita_VertexOffset;
    uint32_t cgVita_VertexLen;
    uint32_t cgVita_PixelOffset;
    uint32_t cgVita_PixelLen;
    uint32_t cgPS3_VertexOffset;
    uint32_t cgPS3_VertexLen;
    uint32_t cgPS3_PixelOffset;
    uint32_t cgPS3_PixelLen;
} Shader;

typedef struct {
    uint32_t count;
    Shader* shaders;
} ShdrChunk;

typedef struct {
    int16_t character;
    int16_t shiftModifier;
} KerningPair;

typedef struct {
    uint16_t character;
    uint16_t sourceX;
    uint16_t sourceY;
    uint16_t sourceWidth;
    uint16_t sourceHeight;
    int16_t shift;
    int16_t offset;
    uint16_t kerningCount;
    KerningPair* kerning;
} FontGlyph;

typedef struct {
    bool present;
    const char* name;
    const char* displayName;
    float emSize;
    bool bold;
    bool italic;
    uint16_t rangeStart;
    uint8_t charset;
    uint8_t antiAliasing;
    uint32_t rangeEnd;
    int32_t tpagIndex;      // resolved TPAG index, -1 if unresolved
    float scaleX;
    float scaleY;
    int32_t ascenderOffset; // wadVersion >= 17 only
    uint32_t ascender;  // GMS 2022.2+ (0 when absent)
    uint32_t sdfSpread; // GMS 2023.2 nonLTS+ (0 when absent)
    uint32_t lineHeight; // GMS 2023.6+ (0 when absent)
    bool hasAscender;
    bool hasSDFSpread;
    bool hasLineHeight;
    uint32_t glyphCount;
    FontGlyph* glyphs;
    uint32_t maxGlyphHeight; // Computed after glyph parse: max sourceHeight across glyphs; HTML5 runner uses this for line stride (see yyFont.TextHeight)
    // ASCII fast-path lookup: glyphLUT[ch] for ch < 128, populated by Font_buildGlyphLUT after glyphs[] is filled.
    // Lets TextUtils_findGlyph skip the linear scan over glyphs[] for the (overwhelmingly common) ASCII case.
    FontGlyph* glyphLUT[128];
    // Sprite font fields (only valid when isSpriteFont is true)
    bool isSpriteFont;
    int32_t spriteIndex; // source sprite index (-1 for regular fonts)
    // Amount to subtract from each glyph's Y at draw time, ONLY used for sprite fonts.
    int16_t spriteOriginYAdjust;
} Font;

typedef struct {
    uint32_t count;
    Font* fonts;
} FontChunk;

// shared by TMLN and OBJT
typedef struct {
    uint32_t libID;
    uint32_t id;
    uint32_t kind;
    bool useRelative;
    bool isQuestion;
    bool useApplyTo;
    uint32_t exeType;
    const char* actionName;
    int32_t codeId;
    uint32_t argumentCount;
    int32_t who;
    bool relative;
    bool isNot;
    uint32_t unknownAlwaysZero;
} EventAction;

typedef struct {
    uint32_t step;
    uint32_t actionCount;
    EventAction* actions;
} TimelineMoment;

typedef struct {
    bool present;
    const char* name;
    uint32_t momentCount;
    TimelineMoment* moments;
} Timeline;

typedef struct {
    uint32_t count;
    Timeline* timelines;
} TmlnChunk;

#define OBJT_EVENT_TYPE_COUNT 15

typedef struct {
    uint32_t eventSubtype;
    uint32_t actionCount;
    EventAction* actions;
} ObjectEvent;

typedef struct {
    uint32_t eventCount;
    ObjectEvent* events;
} ObjectEventList;

typedef struct {
    float x;
    float y;
} PhysicsVertex;

typedef struct {
    bool present;
    const char* name;
    int32_t spriteId;
    bool visible;
    bool managed; // GMS 2022.5+
    bool solid;
    int32_t depth;
    bool persistent;
    int32_t parentId;
    int32_t textureMaskId;
    bool usesPhysics;
    bool isSensor;
    uint32_t collisionShape;
    float density;
    float restitution;
    uint32_t group;
    float linearDamping;
    float angularDamping;
    int32_t physicsVertexCount;
    float friction;
    bool awake;
    bool kinematic;
    PhysicsVertex* physicsVertices;
    ObjectEventList eventLists[OBJT_EVENT_TYPE_COUNT];
} GameObject;

typedef struct {
    uint32_t count;
    GameObject* objects;
} ObjtChunk;

enum {
    RoomLayerType_Background = 0,
    RoomLayerType_Instances = 1,
    RoomLayerType_Tiles = 2,
    RoomLayerType_Path = 3,
    RoomLayerType_Assets = 4,
    RoomLayerType_Effect = 5,
    RoomLayerType_Path2 = 6
};

typedef struct {
    bool enabled;
    bool foreground;
    int32_t backgroundDefinition;
    int32_t x;
    int32_t y;
    int32_t tileX;
    int32_t tileY;
    int32_t speedX;
    int32_t speedY;
    bool stretch;
} RoomBackground;

typedef struct {
    bool enabled;
    int32_t viewX;
    int32_t viewY;
    int32_t viewWidth;
    int32_t viewHeight;
    int32_t portX;
    int32_t portY;
    int32_t portWidth;
    int32_t portHeight;
    uint32_t borderX;
    uint32_t borderY;
    int32_t speedX;
    int32_t speedY;
    int32_t objectId;
} RoomView;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t objectDefinition;
    uint32_t instanceID;
    int32_t creationCode;
    float scaleX;
    float scaleY;
    float imageSpeed;
    int32_t imageIndex;
    uint32_t color;
    float rotation;
    int32_t preCreateCode;
} RoomGameObject;

typedef struct {
    int32_t x;
    int32_t y;
    bool useSpriteDefinition;
    int32_t backgroundDefinition;
    int32_t sourceX;
    int32_t sourceY;
    uint32_t width;
    uint32_t height;
    int32_t tileDepth;
    uint32_t instanceID;
    float scaleX;
    float scaleY;
    uint32_t color;
    float alpha;
} RoomTile;

typedef struct {
    const char* name;
    int32_t spriteIndex;
    int32_t x;
    int32_t y;
    float scaleX;
    float scaleY;
    uint32_t color;
    float animationSpeed;
    uint32_t animationSpeedType;
    float frameIndex;
    float rotation;
} SpriteInstance;

typedef struct {
    uint32_t legacyTileCount;
    RoomTile* legacyTiles;
    uint32_t spriteCount;
    SpriteInstance* sprites;
} RoomLayerAssetsData;

typedef struct {
    bool visible;
    bool foreground;
    int32_t spriteIndex;
    bool hTiled;
    bool vTiled;
    bool stretch;
    uint32_t color;
    float firstFrame;
    float animSpeed;
    uint32_t animSpeedType;
} RoomLayerBackgroundData;

typedef struct {
    uint32_t instanceCount;
    uint32_t* instanceIds;
} RoomLayerInstancesData;

typedef struct {
    int32_t backgroundIndex;
    uint32_t tilesX;
    uint32_t tilesY;
    uint32_t* tileData;
} RoomLayerTilesData;

typedef struct {
    const char* name;
    uint32_t id;
    uint32_t type;
    int32_t depth;
    float xOffset;
    float yOffset;
    float hSpeed;
    float vSpeed;
    bool visible;
    RoomLayerAssetsData* assetsData;
    RoomLayerBackgroundData* backgroundData;
    RoomLayerInstancesData* instancesData;
    RoomLayerTilesData* tilesData;
} RoomLayer;

typedef struct {
    bool present;
    const char* name;
    const char* caption;
    uint32_t width;
    uint32_t height;
    uint32_t speed;
    bool persistent;
    uint32_t backgroundColor;
    bool drawBackgroundColor;
    int32_t creationCodeId;
    uint32_t flags;
    uint32_t backgroundsFileOffset;
    uint32_t viewsFileOffset;
    uint32_t gameObjectsFileOffset;
    uint32_t tilesFileOffset;
    bool world;
    uint32_t top;
    uint32_t left;
    uint32_t right;
    uint32_t bottom;
    float gravityX;
    float gravityY;
    float metersPerPixel;
    uint32_t layersFileOffset;
    bool payloadLoaded;
    bool eagerlyLoaded;
    RoomBackground* backgrounds;
    RoomView* views;
    RoomGameObject* gameObjects;
    uint32_t gameObjectCount;
    RoomTile* tiles;
    uint32_t tileCount;
    RoomLayer* layers;
    uint32_t layerCount;
} Room;

typedef struct {
    uint32_t count;
    Room* rooms;
} RoomChunk;

typedef struct {
    bool present;
    uint16_t sourceX;
    uint16_t sourceY;
    uint16_t sourceWidth;
    uint16_t sourceHeight;
    uint16_t targetX;
    uint16_t targetY;
    uint16_t targetWidth;
    uint16_t targetHeight;
    uint16_t boundingWidth;
    uint16_t boundingHeight;
    int16_t texturePageId;
} TexturePageItem;

typedef struct {
    uint32_t count;
    TexturePageItem* items;
} TpagChunk;

typedef struct {
    const char* name;
    int32_t instanceType;
    int32_t varID;
    uint32_t occurrences;
    uint32_t firstAddress;
} Variable;

typedef struct {
    uint32_t varCount1;
    uint32_t varCount2;
    uint32_t maxLocalVarCount;
    uint32_t variableCount;
    Variable* variables;
} VariChunk;

typedef struct {
    uint32_t varID;
    const char* name;
} LocalVar;

typedef struct {
    uint32_t localVarCount;
    const char* name;
    LocalVar* locals;
} CodeLocals;

typedef struct {
    const char* name;
    uint32_t occurrences;
    uint32_t firstAddress;
} Function;

typedef struct {
    uint32_t functionCount;
    Function* functions;
    uint32_t codeLocalsCount;
    CodeLocals* codeLocals;
} FuncChunk;

typedef struct {
    float x;
    float value;
    float bezierX0;
    float bezierY0;
    float bezierX1;
    float bezierY1;
} AnimCurvePoint;

typedef struct {
    const char* name;
    uint32_t curveType;
    uint32_t iterations;
    uint32_t pointCount;
    AnimCurvePoint* points;
    int32_t globalId;
} AnimCurveChannel;

typedef struct {
    bool present;
    const char* name;
    uint32_t graphType;
    uint32_t channelCount;
    AnimCurveChannel* channels;
    int32_t globalId;
} AnimCurve;

typedef struct {
    uint32_t count;
    AnimCurve* curves;
    AnimCurveChannel** allChannels;
    uint32_t allChannelsCount;
} AcrvChunk;

typedef struct {
    uint32_t count;
    const char** strings;
} StrgChunk;

typedef struct {
    bool present;
    const char* name;
    const char* directory;
    const char* extension;
    int32_t loadType;
    uint32_t texturePageCount;
    int32_t* texturePages;
    uint32_t spriteCount;
    int32_t* sprites;
    uint32_t spineSpriteCount;
    int32_t* spineSprites;
    uint32_t fontCount;
    int32_t* fonts;
    uint32_t tileSetCount;
    int32_t* tilesets;
} TextureGroupInfo;

typedef struct {
    uint32_t count;
    TextureGroupInfo* groups;
} TginChunk;

typedef struct {
    bool present;
    uint32_t scaled;
    uint32_t generatedMips;
    uint32_t textureBlockSize;
    int32_t textureWidth;
    int32_t textureHeight;
    int32_t indexInGroup;
    uint32_t blobOffset;
    uint32_t blobSize;
    uint8_t* blobData;
    bool mapped;
} Texture;

typedef struct {
    uint32_t count;
    Texture* textures;
} TxtrChunk;

typedef struct {
    bool present;
    uint32_t dataSize;
    uint32_t dataOffset;
    uint8_t* data;
} AudioEntry;

typedef struct {
    uint32_t count;
    AudioEntry* entries;
} AudoChunk;

typedef struct {
    bool present;
    const char* name;
    uint32_t length;
    uint16_t localsCount;
    uint16_t argumentsCount;
    uint32_t offset;
    uint32_t bytecodeAbsoluteOffset;
} CodeEntry;

typedef struct {
    uint32_t count;
    CodeEntry* entries;
    uint8_t* bytecodeData;
    uint32_t bytecodeBase;
    size_t bytecodeSize;
} CodeChunk;

typedef struct {
    uint8_t *file_data;
    size_t file_size;
    StringTable strings;
    ChunkTable chunks;

    Gen8Chunk gen8;
    OptnChunk optn;
    LangChunk lang;
    ExtnChunk extn;
    SondChunk sond;
    AgrpChunk agrp;
    SprtChunk sprt;
    BgndChunk bgnd;
    TpagChunk tpag;
    PathChunk path;
    ScptChunk scpt;
    GlobChunk glob;
    CodeChunk code;
    VariChunk vari;
    FuncChunk func;
    ShdrChunk shdr;
    FontChunk font;
    TmlnChunk tmln;
    ObjtChunk objt;
    RoomChunk room;
    AcrvChunk acrv;
    TginChunk tgin;
    StrgChunk strg;
    TxtrChunk txtr;
    AudoChunk audo;

    DetectedFormat detectedFormat;
    uint8_t* mappedFile;

    bool lazyLoadRooms;
    bool lazyLoadTextures;
    bool lazyLoadAudio;
    bool initialized;
} DataWin;

uint8_t *TextureDecode_decodeToRgba(const uint8_t *blob, size_t blob_size, bool gm2022_5, int *out_w, int *out_h);

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
    bool parseTgin;
    bool parseTxtr;
    bool parseAudo;
    bool parseAcrv;
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
int DataWin_loadFile(DataWin *dw, const char *path);
int DataWin_parse(DataWin *dw);
int DataWin_free(DataWin *dw);

bool DataWin_isVersionAtLeast(const DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);
void DataWin_bumpVersionTo(DataWin* dw, uint32_t major, uint32_t minor, uint32_t release, uint32_t build);

#endif // GMDATA_H