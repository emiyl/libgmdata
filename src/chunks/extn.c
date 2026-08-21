#include "common.h"

static int EXTN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int EXTN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
int EXTN_parse(DataWin *dw) {
    Chunk chunk = {0};
    ExtnChunk *e = &dw->extn;

    if (find_chunk(dw, "EXTN", &chunk) != 0) {
        logError("[EXTN_parse] Couldn't find EXTN chunk\n");
        return -1;
    }

    if (chunk.offset + chunk.length > dw->file_size) {
        logError("[EXTN_parse] Chunk length exceeded file size\n");
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "EXTN");

    uint32_t *ptrs = NULL;
    if (Reader_readPointerTable(&reader, &ptrs, &e->count) != 0) {
        logError("[EXTN_parse] Failed to read extension pointer table\n");
        return -1;
    }

    if (e->count == 0) {
        e->extensions = NULL;
        free(ptrs);
        return 0;
    }

    int32_t stringCount = 0;

    if (dw->gen8.wadVersion >= 17 && e->count > 0) {
        uint32_t firstExt = ptrs[0];
        uint32_t value = 0;
        uint32_t filesPtr = 0;

        Reader fileReader;
        Reader_init(&fileReader, dw->file_data, dw->file_size, 0, "EXTN_CHECK");

        // Pointers are given relative to the start of the EXTN chunk, so we need to
        // add the offset of the chunk to get the absolute position in the file.
        uint32_t offset = chunk.offset + firstExt;
        #define PTR_OFFSET(n) (offset + (n) * sizeof(uint32_t))
        
        if (Reader_readUInt32At(&fileReader, PTR_OFFSET(3), &value) == 0 &&
                    value == PTR_OFFSET(5)) {
            stringCount = 3;
        } else if (Reader_readUInt32At(&fileReader, PTR_OFFSET(4), &filesPtr) == 0 &&
                    filesPtr == PTR_OFFSET(6) &&
                    Reader_readUInt32At(&fileReader, PTR_OFFSET(3), &value) == 0 &&
                    value >= 0x1000) {
            stringCount = 4;
        }
    }

    int result = Reader_pointerTable_parse(
        &reader, dw,
        ptrs, e->count,
        (void **)&e->extensions, sizeof(Extension),
        (void *)(uintptr_t)stringCount,
        EXTN_pointerTable_parse,
        EXTN_pointerTable_missingHandler,
        NULL
    );

    free(ptrs);
    return result;
}

static int Extension_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int Extension_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);
static int Extension_parse(Reader *reader, DataWin *dw, Extension *e, int32_t stringCount) {
    if (reader == NULL || e == NULL) {
        logError("[Extension_parse] reader or extension is NULL\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &e->folderName) != 0) {
        logError("[Extension_parse] Failed to read folderName\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &e->name) != 0) {
        logError("[Extension_parse] Failed to read name\n");
        return -1;
    }

    if (stringCount >= 4) {
        if (Reader_skip(reader, 4) != 0) {
            logError("[Extension_parse] Failed to skip version string field\n");
            return -1;
        }
    }

    if (Reader_readString(reader, dw, &e->className) != 0) {
        logError("[Extension_parse] Failed to read className\n");
        return -1;
    }

    Reader fullReader = {0};
    if (stringCount > 0) {
        uint32_t filesPtr = 0;
        if (Reader_readUInt32(reader, &filesPtr) != 0) {
            logError("[Extension_parse] Failed to read files pointer for extension '%s'\n",
                     e->name ? e->name : "<null>");
            return -1;
        }

        Reader_init(&fullReader, dw->file_data, dw->file_size, 0, "EXTN_FILE");
        if (Reader_seek(&fullReader, filesPtr) != 0) {
            logError("[Extension_parse] Failed to seek to files pointer %u for extension '%s'\n",
                     filesPtr, e->name ? e->name : "<null>");
            return -1;
        }
    } else {
        fullReader = *reader;
    }

    uint32_t *ptrs = NULL;
    if (Reader_readPointerTable(&fullReader, &ptrs, &e->fileCount) != 0) {
        logError("[Extension_parse] Failed to read file pointer table for extension '%s'\n",
                 e->name ? e->name : "<null>");
        return -1;
    }

    if (e->fileCount == 0) {
        e->files = NULL;
        free(ptrs);
        return 0;
    }

    int result = Reader_pointerTable_parse(
        &fullReader, dw,
        ptrs, e->fileCount,
        (void **)&e->files, sizeof(ExtensionFile),
        NULL,
        Extension_pointerTable_parse,
        Extension_pointerTable_missingHandler,
        NULL
    );

    free(ptrs);
    return result;
}

static int ExtensionFile_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int ExtensionFunction_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int ExtensionFile_parse(Reader *reader, DataWin *dw, ExtensionFile *f) {
    if (reader == NULL || f == NULL) {
        logError("[ExtensionFile_parse] reader or extension file is NULL\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &f->filename) != 0) {
        logError("[ExtensionFile_parse] Failed to read filename\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &f->cleanupScript) != 0) {
        logError("[ExtensionFile_parse] Failed to read cleanupScript\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &f->initScript) != 0) {
        logError("[ExtensionFile_parse] Failed to read initScript\n");
        return -1;
    }

    if (Reader_readUInt32(reader, &f->kind) != 0) {
        logError("[ExtensionFile_parse] Failed to read kind\n");
        return -1;
    }

    uint32_t *ptrs = NULL;
    if (Reader_readPointerTable(reader, &ptrs, &f->functionCount) != 0) {
        logError("[ExtensionFile_parse] Failed to read function pointer table\n");
        return -1;
    }

    if (f->functionCount == 0) {
        f->functions = NULL;
        free(ptrs);
        return 0;
    }

    int result = Reader_pointerTable_parse(
        reader, dw,
        ptrs, f->functionCount,
        (void **)&f->functions, sizeof(ExtensionFunction),
        NULL,
        ExtensionFile_pointerTable_parse,
        ExtensionFunction_pointerTable_parse,
        NULL
    );

    free(ptrs);
    return result;
}

static int ExtensionFunction_parse(Reader *reader, DataWin *dw, ExtensionFunction *func) {
    if (reader == NULL || func == NULL) {
        logError("[ExtensionFunction_parse] reader or extension function is NULL\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &func->name) != 0) {
        logError("[ExtensionFunction_parse] Failed to read name\n");
        return -1;
    }

    if (Reader_readUInt32(reader, &func->id) != 0) {
        logError("[ExtensionFunction_parse] Failed to read id\n");
        return -1;
    }

    if (Reader_readUInt32(reader, &func->kind) != 0) {
        logError("[ExtensionFunction_parse] Failed to read kind\n");
        return -1;
    }

    if (Reader_readUInt32(reader, &func->retType) != 0) {
        logError("[ExtensionFunction_parse] Failed to read retType\n");
        return -1;
    }

    if (Reader_readString(reader, dw, &func->extName) != 0) {
        logError("[ExtensionFunction_parse] Failed to read extName\n");
        return -1;
    }

    if (Reader_readUInt32(reader, &func->argumentCount) != 0) {
        logError("[ExtensionFunction_parse] Failed to read argumentCount\n");
        return -1;
    }

    if (func->argumentCount == 0) {
        func->arguments = NULL;
        return 0;
    }

    func->arguments = (uint32_t *)safeMalloc(sizeof(uint32_t) * func->argumentCount);

    for (uint32_t i = 0; i < func->argumentCount; ++i) {
        if (Reader_readUInt32(reader, &func->arguments[i]) != 0) {
            logError("[ExtensionFunction_parse] Failed to read argument[%u]\n", i);
            free(func->arguments);
            func->arguments = NULL;
            return -1;
        }
    }

    return 0;
}

static int EXTN_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    int32_t stringCount = (int32_t)(uintptr_t)extraData;
    return Extension_parse(reader, dw, (Extension *)out, stringCount);
}

static int EXTN_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[EXTN_pointerTable_missingHandler] Extension pointer is missing, initializing default values.\n");

    Extension *ext = (Extension *)out;
    ext->folderName = NULL;
    ext->name = NULL;
    ext->className = NULL;
    ext->fileCount = 0;
    ext->files = NULL;

    return 0;
}

static int Extension_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return ExtensionFile_parse(reader, dw, (ExtensionFile *)out);
}

static int Extension_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; // Unused parameter
    (void)dw;     // Unused parameter
    (void)extraData; // Unused parameter

    logWarn("[Extension_pointerTable_missingHandler] Extension file pointer is missing, initializing default values.\n");

    ExtensionFile *file = (ExtensionFile *)out;
    file->filename = NULL;
    file->cleanupScript = NULL;
    file->initScript = NULL;
    file->kind = 0;
    file->functionCount = 0;
    file->functions = NULL;

    return 0;
}

static int ExtensionFile_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return ExtensionFunction_parse(reader, dw, (ExtensionFunction *)out);
}

static int ExtensionFunction_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return ExtensionFunction_parse(reader, dw, (ExtensionFunction *)out);
}

static void ExtensionFunction_free(ExtensionFunction *func) {
    if (func == NULL) {
        return;
    }

    free(func->arguments);
}

static void ExtensionFile_free(ExtensionFile *file) {
    if (file == NULL) {
        return;
    }

    for (uint32_t i = 0; i < file->functionCount; ++i) {
        ExtensionFunction_free(&file->functions[i]);
    }

    free(file->functions);
}

static void Extension_free(Extension *ext) {
    if (ext == NULL) {
        return;
    }

    for (uint32_t i = 0; i < ext->fileCount; ++i) {
        ExtensionFile_free(&ext->files[i]);
    }

    free(ext->files);
}

void EXTN_free(ExtnChunk *e) {
    if (e == NULL) {
        return;
    }

    for (uint32_t i = 0; i < e->count; ++i) {
        Extension_free(&e->extensions[i]);
    }

    free(e->extensions);
}