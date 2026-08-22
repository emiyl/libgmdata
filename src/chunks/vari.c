#include "common.h"
#include "../datawin.h"

int VARI_free(VariChunk *v);

int VARI_parse(DataWin *dw) {
    Chunk chunk = {0};
    VariChunk *v = &dw->vari;

    if (get_chunk(dw, "VARI", &chunk) != 0) {
        v->varCount1 = 0;
        v->varCount2 = 0;
        v->maxLocalVarCount = 0;
        v->variableCount = 0;
        v->variables = NULL;
        return 0;
    }
    if (chunk.offset + chunk.length > dw->file_size) {
        return -1;
    }

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "VARI");

    bool oldFormat = dw->gen8.wadVersion <= 14;
    if (oldFormat) {
        v->varCount1 = 0;
        v->varCount2 = 0;
        v->maxLocalVarCount = 0;
        v->variableCount = chunk.length / 12U;
        if (v->variableCount == 0) {
            v->variables = NULL;
            return 0;
        }

        v->variables = (Variable *)safeMalloc(v->variableCount * sizeof(Variable));
        repeat(v->variableCount, i) {
            Variable *var = &v->variables[i];
            if (Reader_readString(&reader, dw, &var->name) != 0) {
                VARI_free(v);
                return -1;
            }
            if (Reader_readUInt32(&reader, &var->occurrences) != 0) {
                VARI_free(v);
                return -1;
            }
            if (Reader_readUInt32(&reader, &var->firstAddress) != 0) {
                VARI_free(v);
                return -1;
            }
            var->instanceType = 0;
            var->varID = 0;
        }
        return 0;
    }

    if (Reader_readUInt32(&reader, &v->varCount1) != 0) return -1;
    if (Reader_readUInt32(&reader, &v->varCount2) != 0) return -1;
    if (Reader_readUInt32(&reader, &v->maxLocalVarCount) != 0) return -1;

    if (chunk.length < 12U) {
        v->variableCount = 0;
        v->variables = NULL;
        return 0;
    }

    v->variableCount = (chunk.length - 12U) / 20U;
    if (v->variableCount == 0) {
        v->variables = NULL;
        return 0;
    }

    v->variables = (Variable *)safeMalloc(v->variableCount * sizeof(Variable));
    repeat(v->variableCount, i) {
        Variable *var = &v->variables[i];
        if (Reader_readString(&reader, dw, &var->name) != 0) {
            VARI_free(v);
            return -1;
        }
        if (Reader_readInt32(&reader, &var->instanceType) != 0) {
            VARI_free(v);
            return -1;
        }
        if (Reader_readInt32(&reader, &var->varID) != 0) {
            VARI_free(v);
            return -1;
        }
        if (Reader_readUInt32(&reader, &var->occurrences) != 0) {
            VARI_free(v);
            return -1;
        }
        if (Reader_readUInt32(&reader, &var->firstAddress) != 0) {
            VARI_free(v);
            return -1;
        }
    }

    return 0;
}

int VARI_free(VariChunk *v) {
    if (v == NULL) {
        return -1;
    }

    if (v->variables != NULL) {
        repeat(v->variableCount, i) {
            free((void *)v->variables[i].name);
            v->variables[i].name = NULL;
        }
        free(v->variables);
        v->variables = NULL;
    }

    v->varCount1 = 0;
    v->varCount2 = 0;
    v->maxLocalVarCount = 0;
    v->variableCount = 0;
    return 0;
}
