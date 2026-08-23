#include "common.h"

static int VARI_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
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
    Reader re; Reader* reader = &re;
    Reader_init(reader, base, chunk.length, chunk.offset, "VARI");

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

        uint32_t *ptrs = (uint32_t *)safeMalloc(v->variableCount * sizeof(uint32_t));
        repeat(v->variableCount, i) {
            ptrs[i] = (uint32_t)(i * 12U);
        }

        int result = Reader_parsePointerTableParallel(
            reader, dw,
            ptrs, v->variableCount,
            (void **)&v->variables, sizeof(Variable),
            NULL,
            VARI_pointerTable_parse,
            NULL,
            NULL
        );

        free(ptrs);
        return result;
    }

    read(&v->varCount1, UInt32);
    read(&v->varCount2, UInt32);
    read(&v->maxLocalVarCount, UInt32);

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

    uint32_t *ptrs = (uint32_t *)safeMalloc(v->variableCount * sizeof(uint32_t));
    repeat(v->variableCount, i) {
        ptrs[i] = 12U + (uint32_t)(i * 20U);
    }

    int result = Reader_parsePointerTable(
        reader, dw,
        ptrs, v->variableCount,
        (void **)&v->variables, sizeof(Variable),
        NULL,
        VARI_pointerTable_parse,
        NULL,
        NULL
    );

    free(ptrs);
    return result;
}

static int VARI_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    Variable *var = (Variable *)out;
    memset(var, 0, sizeof(*var));

    if (dw->gen8.wadVersion <= 14) {
        readString(&var->name, dw);
        read(&var->occurrences, UInt32);
        read(&var->firstAddress, UInt32);
        var->instanceType = 0;
        var->varID = 0;
        return 0;
    }

    readString(&var->name, dw);
    read(&var->instanceType, Int32);
    read(&var->varID, Int32);
    read(&var->occurrences, UInt32);
    read(&var->firstAddress, UInt32);

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
