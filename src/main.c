#include "datawin.h"
#include "chunks/gen8.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data.win>\n", argv[0]);
        return 1;
    }

    DataWin dw = {0};
    if (load_file(&dw, argv[1]) != 0) {
        fprintf(stderr, "failed to load file: %s\n", argv[1]);
        return 1;
    }

    if (parse(&dw) != 0) {
        fprintf(stderr, "failed to parse file: %s\n", argv[1]);
        datawin_free(&dw);
        return 1;
    }

    printf("Loaded %zu bytes with %zu chunks\n", dw.file_size, dw.chunks.count);
    Gen8_Print(dw.gen8);
    // Gen8_Bytedump(&dw);

    datawin_free(&dw);
    return 0;
}