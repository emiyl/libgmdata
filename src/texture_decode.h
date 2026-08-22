#ifndef GMDATA_TEXTURE_DECODE_H
#define GMDATA_TEXTURE_DECODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint8_t *TextureDecode_decodeToRgba(const uint8_t *blob, size_t blob_size, bool gm2022_5, int *out_w, int *out_h);

#endif
