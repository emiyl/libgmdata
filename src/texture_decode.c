#include "texture_decode.h"
#include "log.h"

#include <bzlib.h>
#include <stdlib.h>
#include <string.h>

#define QOI_HEADER_SIZE 12
#define COMPRESSED_QOI_HEADER_SIZE_OLD 8
#define COMPRESSED_QOI_HEADER_SIZE_NEW 12

static inline uint8_t sign_extend(uint32_t value, int bits) {
    uint32_t mask = 1U << (bits - 1);
    return (uint8_t)((value ^ mask) - mask);
}

static uint8_t *decode_qoi_rgba(const uint8_t *data, size_t data_size, int *out_w, int *out_h) {
    if (data == NULL || data_size < QOI_HEADER_SIZE) {
        logError("decode_qoi_rgba: Invalid input: data is NULL or data_size is less than QOI_HEADER_SIZE\n");
        return NULL;
    }
    if (data[0] != 'f' || data[1] != 'i' || data[2] != 'o' || data[3] != 'q') {
        logError("decode_qoi_rgba: Invalid QOI header: expected 'fioq', got '%c%c%c%c'\n", data[0], data[1], data[2], data[3]);
        return NULL;
    }

    int width = data[4] | (data[5] << 8);
    int height = data[6] | (data[7] << 8);
    uint32_t length = (uint32_t)data[8] | ((uint32_t)data[9] << 8) | ((uint32_t)data[10] << 16) | ((uint32_t)data[11] << 24);

    if (QOI_HEADER_SIZE + (size_t)length > data_size) {
        logError("decode_qoi_rgba: Invalid QOI data: length field exceeds available data size\n");
        return NULL;
    }
    if (width <= 0 || height <= 0) {
        logError("decode_qoi_rgba: Invalid QOI dimensions: width=%d, height=%d\n", width, height);
        return NULL;
    }

    const uint8_t *pixel_data = data + QOI_HEADER_SIZE;
    size_t raw_size = (size_t)width * (size_t)height * 4;
    uint8_t *raw = (uint8_t *)malloc(raw_size);
    if (raw == NULL) {
        return NULL;
    }

    uint8_t index[64 * 4];
    memset(index, 0, sizeof(index));

    size_t pos = 0;
    int run = 0;
    uint8_t r = 0, g = 0, b = 0, a = 255;

    for (size_t raw_data_pos = 0; raw_size > raw_data_pos; raw_data_pos += 4) {
        if (run > 0) {
            run--;
        } else if (length > pos) {
            uint8_t b1 = pixel_data[pos++];

            if ((b1 & 0xC0U) == 0x00U) {
                int index_pos = (b1 & 0x3FU) << 2;
                r = index[index_pos];
                g = index[index_pos + 1];
                b = index[index_pos + 2];
                a = index[index_pos + 3];
            } else if ((b1 & 0xE0U) == 0x40U) {
                run = b1 & 0x1FU;
            } else if ((b1 & 0xE0U) == 0x60U) {
                if (length <= pos) {
                    logError("decode_qoi_rgba: Unexpected end of data while reading run length\n");
                    free(raw);
                    return NULL;
                }
                uint8_t b2 = pixel_data[pos++];
                run = (((b1 & 0x1FU) << 8) | b2) + 32;
            } else if ((b1 & 0xC0U) == 0x80U) {
                r += sign_extend((b1 >> 4) & 3U, 2);
                g += sign_extend((b1 >> 2) & 3U, 2);
                b += sign_extend(b1 & 3U, 2);
            } else if ((b1 & 0xE0U) == 0xC0U) {
                if (length <= pos) {
                    logError("decode_qoi_rgba: Unexpected end of data while reading 2-byte pixel data\n");
                    free(raw);
                    return NULL;
                }
                uint8_t b2 = pixel_data[pos++];
                uint32_t merged = ((uint32_t)b1 << 8) | b2;
                r += sign_extend((merged >> 8) & 0x1FU, 5);
                g += sign_extend((merged >> 4) & 0x0FU, 4);
                b += sign_extend(merged & 0x0FU, 4);
            } else if ((b1 & 0xF0U) == 0xE0U) {
                if (length <= pos + 1U) {
                    free(raw);
                    return NULL;
                }
                uint8_t b2 = pixel_data[pos++];
                uint8_t b3 = pixel_data[pos++];
                uint32_t merged = ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | b3;
                r += sign_extend((merged >> 15) & 0x1FU, 5);
                g += sign_extend((merged >> 10) & 0x1FU, 5);
                b += sign_extend((merged >> 5) & 0x1FU, 5);
                a += sign_extend(merged & 0x1FU, 5);
            } else if ((b1 & 0xF0U) == 0xF0U) {
                if (b1 & 8U) {
                    if (length <= pos) {
                        free(raw);
                        return NULL;
                    }
                    r = pixel_data[pos++];
                }
                if (b1 & 4U) {
                    if (length <= pos) {
                        free(raw);
                        return NULL;
                    }
                    g = pixel_data[pos++];
                }
                if (b1 & 2U) {
                    if (length <= pos) {
                        free(raw);
                        return NULL;
                    }
                    b = pixel_data[pos++];
                }
                if (b1 & 1U) {
                    if (length <= pos) {
                        free(raw);
                        return NULL;
                    }
                    a = pixel_data[pos++];
                }
            }

            int index_pos = ((r ^ g ^ b ^ a) & 63) << 2;
            index[index_pos] = r;
            index[index_pos + 1] = g;
            index[index_pos + 2] = b;
            index[index_pos + 3] = a;
        }

        raw[raw_data_pos] = r;
        raw[raw_data_pos + 1] = g;
        raw[raw_data_pos + 2] = b;
        raw[raw_data_pos + 3] = a;
    }

    if (out_w != NULL) {
        *out_w = width;
    }
    if (out_h != NULL) {
        *out_h = height;
    }

    return raw;
}

static uint8_t *decode_bz2_qoi_rgba(const uint8_t *blob, size_t blob_size, bool gm2022_5, int *out_w, int *out_h) {
    if (blob == NULL || blob_size == 0) {
        logError("decode_bz2_qoi_rgba: Invalid input: blob is NULL or blob_size is 0\n");
        return NULL;
    }

    size_t header_size = gm2022_5 ? COMPRESSED_QOI_HEADER_SIZE_NEW : COMPRESSED_QOI_HEADER_SIZE_OLD;
    if (header_size > blob_size) {
        return NULL;
    }

    int width = (int)(blob[4] | ((uint16_t)blob[5] << 8));
    int height = (int)(blob[6] | ((uint16_t)blob[7] << 8));
    if (width <= 0 || height <= 0) {
        return NULL;
    }

    size_t uncompressed_capacity = QOI_HEADER_SIZE + (size_t)width * (size_t)height * 5U;
    uint8_t *uncompressed = (uint8_t *)malloc(uncompressed_capacity);
    if (uncompressed == NULL) {
        return NULL;
    }

    unsigned int dest_len = (unsigned int)uncompressed_capacity;
    int rc = BZ2_bzBuffToBuffDecompress(
        (char *)uncompressed,
        &dest_len,
        (char *)(blob + header_size),
        (unsigned int)(blob_size - header_size),
        0,
        0
    );
    if (rc != BZ_OK) {
        logError("decode_bz2_qoi_rgba: BZ2 decompression failed with error code %d\n", rc);
        free(uncompressed);
        return NULL;
    }

    uint8_t *result = decode_qoi_rgba(uncompressed, dest_len, out_w, out_h);
    free(uncompressed);
    return result;
}

uint8_t *TextureDecode_decodeToRgba(const uint8_t *blob, size_t blob_size, bool gm2022_5, int *out_w, int *out_h) {
    if (blob == NULL || blob_size < 4U) {
        return NULL;
    }

    if (blob[0] == 'f' && blob[1] == 'i' && blob[2] == 'o' && blob[3] == 'q') {
        return decode_qoi_rgba(blob, blob_size, out_w, out_h);
    }

    if (blob[0] == '2' && blob[1] == 'z' && blob[2] == 'o' && blob[3] == 'q') {
        return decode_bz2_qoi_rgba(blob, blob_size, gm2022_5, out_w, out_h);
    }

    if (blob_size % 4U != 0U) {
        logError("TextureDecode_decodeToRgba: Invalid raw RGBA data size: %zu\n", blob_size);
        return NULL;
    }

    size_t pixel_count = blob_size / 4U;
    uint8_t *rgba = (uint8_t *)malloc(pixel_count * 4U);
    if (rgba == NULL) {
        logError("TextureDecode_decodeToRgba: Memory allocation failed for raw RGBA data\n");
        return NULL;
    }

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t b = blob[i * 4U + 0U];
        uint8_t g = blob[i * 4U + 1U];
        uint8_t r = blob[i * 4U + 2U];
        uint8_t a = blob[i * 4U + 3U];
        rgba[i * 4U + 0U] = r;
        rgba[i * 4U + 1U] = g;
        rgba[i * 4U + 2U] = b;
        rgba[i * 4U + 3U] = a;
    }

    if (out_w != NULL) {
        *out_w = 0;
    }
    if (out_h != NULL) {
        *out_h = 0;
    }

    return rgba;
}
