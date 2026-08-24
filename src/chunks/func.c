#include "common.h"

int FUNC_free(FuncChunk *f);

static int Function_parse(Reader *reader, DataWin *dw, Function *func);
static int CodeLocals_parse(Reader *reader, DataWin *dw, CodeLocals *locals);

int FUNC_parse(DataWin *dw) {
    Chunk chunk = {0};
    FuncChunk *f = &dw->func;

    if (get_chunk(dw, "FUNC", &chunk) != 0) {
        f->functionCount = 0;
        f->functions = NULL;
        f->codeLocalsCount = 0;
        f->codeLocals = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader re; Reader* reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "FUNC");

    if (dw->gen8.wadVersion <= 14) {
        f->functionCount = chunk.length / 12U;
        if (f->functionCount == 0) {
            f->functions = NULL;
            f->codeLocalsCount = 0;
            f->codeLocals = NULL;
            return 0;
        }

        f->functions = (Function *)safeMalloc(f->functionCount * sizeof(Function));
        repeat(f->functionCount, i) {
            if (Function_parse(reader, dw, &f->functions[i]) != 0) {
                FUNC_free(f);
                return -1;
            }
        }
        f->codeLocalsCount = 0;
        f->codeLocals = NULL;
        return 0;
    }

    read(&f->functionCount, UInt32);
    if (f->functionCount > 0) {
        f->functions = (Function *)safeMalloc(f->functionCount * sizeof(Function));
        repeat(f->functionCount, i) {
            if (Function_parse(reader, dw, &f->functions[i]) != 0) {
                FUNC_free(f);
                return -1;
            }
        }
    } else {
        f->functions = NULL;
    }

    if (DataWin_isVersionAtLeast(dw, 2024, 8, 0, 0)) {
        f->codeLocalsCount = 0;
        f->codeLocals = NULL;
        return 0;
    }

    read(&f->codeLocalsCount, UInt32);
    if (f->codeLocalsCount > 0) {
        f->codeLocals = (CodeLocals *)safeMalloc(f->codeLocalsCount * sizeof(CodeLocals));
        repeat(f->codeLocalsCount, i) {
            if (CodeLocals_parse(reader, dw, &f->codeLocals[i]) != 0) {
                FUNC_free(f);
                return -1;
            }
        }
    } else {
        f->codeLocals = NULL;
    }

    return 0;
}

static int Function_parse(Reader *reader, DataWin *dw, Function *func) {
    if (reader == NULL || func == NULL) {
        return -1;
    }

    memset(func, 0, sizeof(*func));
    readString(&func->name, dw);
    read(&func->occurrences, UInt32);
    uint32_t rawAddr = 0;
    read(&rawAddr, UInt32);
    if (DataWin_isVersionAtLeast(dw, 2, 3, 0, 0) && rawAddr != (uint32_t)-1) {
        rawAddr -= 4U;
    }
    func->firstAddress = rawAddr;
    return 0;
}

static int CodeLocals_parse(Reader *reader, DataWin *dw, CodeLocals *locals) {
    if (reader == NULL || locals == NULL) {
        return -1;
    }

    memset(locals, 0, sizeof(*locals));
    read(&locals->localVarCount, UInt32);
    readString(&locals->name, dw);

    if (locals->localVarCount > 0) {
        locals->locals = (LocalVar *)safeMalloc(locals->localVarCount * sizeof(LocalVar));
        repeat(locals->localVarCount, j) {
            if (Reader_readUInt32(reader, &locals->locals[j].varID) != 0) {
                repeat(j, k) {
                    free((void *)locals->locals[k].name);
                    locals->locals[k].name = NULL;
                }
                free(locals->locals);
                locals->locals = NULL;
                return -1;
            }
            if (Reader_readString(reader, dw, &locals->locals[j].name) != 0) {
                repeat(j + 1U, k) {
                    free((void *)locals->locals[k].name);
                    locals->locals[k].name = NULL;
                }
                free(locals->locals);
                locals->locals = NULL;
                return -1;
            }
        }
    } else {
        locals->locals = NULL;
    }
    return 0;
}

int FUNC_free(FuncChunk *f) {
    if (f == NULL) {
        return -1;
    }

    if (f->functions != NULL) {
        repeat(f->functionCount, i) {
            free((void *)f->functions[i].name);
            f->functions[i].name = NULL;
        }
        free(f->functions);
        f->functions = NULL;
    }

    if (f->codeLocals != NULL) {
        repeat(f->codeLocalsCount, i) {
            if (f->codeLocals[i].locals != NULL) {
                repeat(f->codeLocals[i].localVarCount, j) {
                    free((void *)f->codeLocals[i].locals[j].name);
                    f->codeLocals[i].locals[j].name = NULL;
                }
                free(f->codeLocals[i].locals);
                f->codeLocals[i].locals = NULL;
            }
            free((void *)f->codeLocals[i].name);
            f->codeLocals[i].name = NULL;
        }
        free(f->codeLocals);
        f->codeLocals = NULL;
    }

    f->functionCount = 0;
    f->codeLocalsCount = 0;
    return 0;
}
