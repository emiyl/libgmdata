#include "common.h"

static int ACRV_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData);
static int ACRV_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData);

static int AnimCurvePoint_parse(Reader *reader, AnimCurvePoint *point) {
    read(&point->x, Float32);
    read(&point->value, Float32);
    read(&point->bezierX0, Float32);
    read(&point->bezierY0, Float32);
    read(&point->bezierX1, Float32);
    read(&point->bezierY1, Float32);
    return 0;
}

static int AnimCurveChannel_parse(Reader *reader, DataWin *dw, AnimCurveChannel *channel) {
    (void)dw;

    readString(&channel->name, dw);
    read(&channel->curveType, UInt32);
    read(&channel->iterations, UInt32);
    read(&channel->pointCount, UInt32);

    channel->points = NULL;
    if (channel->pointCount > 0) {
        channel->points = (AnimCurvePoint *)safeMalloc(channel->pointCount * sizeof(AnimCurvePoint));
        repeat(channel->pointCount, i) {
            if (AnimCurvePoint_parse(reader, &channel->points[i]) != 0) {
                free(channel->points);
                channel->points = NULL;
                return -1;
            }
        }
    }

    return 0;
}

static int AnimCurve_parse(Reader *reader, DataWin *dw, AnimCurve *curve) {
    readString(&curve->name, dw);
    read(&curve->graphType, UInt32);
    read(&curve->channelCount, UInt32);

    curve->channels = NULL;
    if (curve->channelCount > 0) {
        curve->channels = (AnimCurveChannel *)safeCalloc(curve->channelCount, sizeof(AnimCurveChannel));
        repeat(curve->channelCount, i) {
            if (AnimCurveChannel_parse(reader, dw, &curve->channels[i]) != 0) {
                free(curve->channels);
                curve->channels = NULL;
                return -1;
            }
        }
    }

    curve->present = true;
    curve->globalId = -1;
    return 0;
}

int ACRV_parse(DataWin *dw) {
    Chunk chunk = {0};
    AcrvChunk *a = &dw->acrv;

    if (get_chunk(dw, "ACRV", &chunk) != 0) return -1;
    if (chunk.offset + chunk.length > dw->file_size) return -1;

    const uint8_t *base = dw->file_data + chunk.offset;
    Reader reader;
    Reader_init(&reader, base, chunk.length, chunk.offset, "ACRV");

    return Reader_readAndParsePointerTable(
        &reader, dw,
        (void **)&a->curves, NULL,
        &a->count, sizeof(AnimCurve),
        ACRV_pointerTable_parse,
        ACRV_pointerTable_missingHandler,
        NULL
    );
}

static int ACRV_pointerTable_parse(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)extraData;
    return AnimCurve_parse(reader, dw, (AnimCurve *)out);
}

static int ACRV_pointerTable_missingHandler(Reader *reader, DataWin *dw, void *out, void *extraData) {
    (void)reader; (void)dw; (void)extraData;
    AnimCurve *curve = (AnimCurve *)out;
    memset(curve, 0, sizeof(*curve));
    curve->present = false;
    return 0;
}

static void ACRV_free_curve(AnimCurve *curve) {
    if (curve == NULL) return;
    if (curve->channels != NULL) {
        repeat(curve->channelCount, i) {
            free(curve->channels[i].points);
            curve->channels[i].points = NULL;
            curve->channels[i].pointCount = 0;
            free((void *)curve->channels[i].name);
            curve->channels[i].name = NULL;
        }
        free(curve->channels);
        curve->channels = NULL;
    }
    free((void *)curve->name);
    curve->name = NULL;
    curve->channelCount = 0;
    curve->present = false;
}

int ACRV_free(AcrvChunk *a) {
    if (a == NULL) return -1;
    if (a->curves != NULL) {
        repeat(a->count, i) {
            ACRV_free_curve(&a->curves[i]);
        }
        free(a->curves);
    }
    free(a->allChannels);
    a->curves = NULL;
    a->allChannels = NULL;
    a->allChannelsCount = 0;
    a->count = 0;
    return 0;
}
