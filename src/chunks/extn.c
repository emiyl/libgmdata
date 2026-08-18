#include "common.h"

int Extension_parse(Reader *reader, DataWin *dw, Extension *ext, int32_t stringCount);
int ExtensionFile_parse(Reader *reader, DataWin *dw, ExtensionFile *file);
int ExtensionFunction_parse(Reader *reader, DataWin *dw, ExtensionFunction *func);

int EXTN_parse(DataWin *dw) {
    Chunk chunk = {0};
    Extn *e = &dw->extn;

    if (find_chunk(dw, "EXTN", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;

    Reader reader;
    Reader_init(&reader, base, chunk.length, "EXTN");

    uint32_t extCount;
    uint32_t *extPtrs;
    if (Reader_readPointerTable(&reader, &extPtrs, &extCount) != 0) return -1;

    e->count = extCount;

    if (extCount == 0) {
        free(extPtrs);
        e->extensions = NULL;
        return 0;
    }

    int32_t extStringCount = 0;
    if (dw->gen8.wadVersion >= 17) {
        uint32_t firstExt = extPtrs[0];
        uint32_t value;

        // 2022.6: [folder][name][className][filesPtr][optionsPtr][files list...]
        if (Reader_readUInt32At(&reader, firstExt + 12, &value) == 0 &&
            value == firstExt + 20) {
            extStringCount = 3;
        }
        // 2023.4+: an extra Version string sits between name and className.
        // Check +12 >= 0x1000 to distinguish this from old extensions
        // where +12 is a small file count.
        else {
            uint32_t filesPtr;

            if (Reader_readUInt32At(&reader, firstExt + 12, &value) == 0 &&
                value >= 0x1000 &&
                Reader_readUInt32At(&reader, firstExt + 16, &filesPtr) == 0 &&
                filesPtr == firstExt + 24) {
                extStringCount = 4;
            }
        }
    }

    e->extensions = (Extension *)safeMalloc(sizeof(Extension) * extCount);
    repeat(extCount, i) {
        Reader_seek(&reader, extPtrs[i] - chunk.offset);
        Extension* ext = &e->extensions[i];
        Extension_parse(&reader, dw, ext, extStringCount);
    }

    return 0;
}

int Extension_parse(Reader *reader, DataWin *dw, Extension *ext, int32_t stringCount) {
    if (reader == NULL || ext == NULL) {
        return -1;
    }

    Reader_readString(reader, dw, &ext->folderName);
    Reader_readString(reader, dw, &ext->name);

    if (stringCount >= 4) Reader_skip(reader, 4); // Skip Version string if present
    Reader_readString(reader, dw, &ext->className);

    if (stringCount > 0) {
        uint32_t filesPtr;
        Reader_readUInt32(reader, &filesPtr);
        Reader_seek(reader, filesPtr);
    }

    uint32_t fileCount;
    uint32_t *filePtrs;
    if (Reader_readPointerTable(reader, &filePtrs, &fileCount) != 0) return -1;
    ext->fileCount = fileCount;

    if (fileCount == 0) {
        free(filePtrs);
        ext->files = NULL;
        return 0;
    }

    ext->files = (ExtensionFile *)safeMalloc(sizeof(ExtensionFile) * fileCount);
    repeat(fileCount, i) {
        Reader_seek(reader, filePtrs[i]);
        ExtensionFile *file = &ext->files[i];
        ExtensionFile_parse(reader, dw, file);
    }

    return 0;
}

int ExtensionFile_parse(Reader *reader, DataWin *dw, ExtensionFile *file) {
    if (reader == NULL || file == NULL) {
        return -1;
    }

    Reader_readString(reader, dw, &file->filename);
    Reader_readString(reader, dw, &file->cleanupScript);
    Reader_readString(reader, dw, &file->initScript);
    Reader_readUInt32(reader, &file->kind);

    uint32_t funcCount;
    uint32_t *funcPtrs;
    if (Reader_readPointerTable(reader, &funcPtrs, &funcCount) != 0) return -1;
    file->functionCount = funcCount;
    
    if (funcCount == 0) {
        free(funcPtrs);
        file->functionCount = 0;
        file->functions = NULL;
        return 0;
    }

    file->functions = (ExtensionFunction *)safeMalloc(sizeof(ExtensionFunction) * funcCount);
    repeat(funcCount, i) {
        Reader_seek(reader, funcPtrs[i]);
        ExtensionFunction *func = &file->functions[i];
        ExtensionFunction_parse(reader, dw, func);
    }

    return 0;
}

int ExtensionFunction_parse(Reader *reader, DataWin *dw, ExtensionFunction *func) {
    if (reader == NULL || func == NULL) {
        return -1;
    }

    Reader_readString(reader, dw, &func->name);
    Reader_readUInt32(reader, &func->id);
    Reader_readUInt32(reader, &func->kind);
    Reader_readUInt32(reader, &func->retType);
    Reader_readString(reader, dw, &func->extName);

    Reader_readUInt32(reader, &func->argumentCount);
    if (func->argumentCount == 0) {
        func->arguments = NULL;
        return 0;
    }

    func->arguments = (uint32_t *)safeMalloc(sizeof(uint32_t) * func->argumentCount);
    repeat(func->argumentCount, i) {
        Reader_readUInt32(reader, &func->arguments[i]);
    }

    return 0;
}

void ExtensionFunction_free(ExtensionFunction *func) {
    if (func == NULL) return;

    free(func->arguments);
}

void ExtensionFile_free(ExtensionFile *file) {
    if (file == NULL) return;

    repeat(file->functionCount, i) {
        ExtensionFunction_free(&file->functions[i]);
    }
    free(file->functions);
}

void Extension_free(Extension *ext) {
    if (ext == NULL) return;

    repeat(ext->fileCount, i) {
        ExtensionFile_free(&ext->files[i]);
    }
    free(ext->files);
}

void EXTN_free(Extn *e) {
    if (e == NULL) return;

    repeat(e->count, i) {
        Extension_free(&e->extensions[i]);
    }
    free(e->extensions);
}