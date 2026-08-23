#include "src/gmdata.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    char name[32];
    uint64_t total_ns;
    uint64_t max_ns;
    size_t count;
} ChunkTiming;

typedef struct {
    uint64_t parse_started_ns;
    size_t chunk_count;
    size_t chunk_capacity;
    ChunkTiming *chunks;
} BenchmarkTimingState;

static uint64_t elapsed_ns(const struct timespec *start, const struct timespec *end) {
    uint64_t start_ns = (uint64_t)start->tv_sec * 1000000000ULL + (uint64_t)start->tv_nsec;
    uint64_t end_ns = (uint64_t)end->tv_sec * 1000000000ULL + (uint64_t)end->tv_nsec;
    if (end_ns < start_ns) {
        return 0;
    }
    return end_ns - start_ns;
}

static uint64_t monotonic_ns_now(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

static void timing_state_reset(BenchmarkTimingState *state) {
    if (state == NULL) {
        return;
    }

    state->parse_started_ns = 0;
    state->chunk_count = 0;
}

static ChunkTiming *timing_state_find_or_add(BenchmarkTimingState *state, const char *name) {
    size_t i;
    ChunkTiming *entry;

    if (state == NULL || name == NULL || *name == '\0') {
        return NULL;
    }

    for (i = 0; i < state->chunk_count; ++i) {
        if (strcmp(state->chunks[i].name, name) == 0) {
            return &state->chunks[i];
        }
    }

    if (state->chunk_count == state->chunk_capacity) {
        size_t new_capacity = state->chunk_capacity == 0 ? 8 : state->chunk_capacity * 2;
        ChunkTiming *new_chunks = (ChunkTiming *)realloc(state->chunks, new_capacity * sizeof(*new_chunks));
        if (new_chunks == NULL) {
            return NULL;
        }
        state->chunks = new_chunks;
        state->chunk_capacity = new_capacity;
    }

    entry = &state->chunks[state->chunk_count++];
    memset(entry, 0, sizeof(*entry));
    snprintf(entry->name, sizeof(entry->name), "%s", name);
    return entry;
}

static void benchmark_progress_callback(
    const char *chunkName,
    int chunkIndex,
    int totalChunks,
    DataWin *dataWin,
    void *userData
) {
    BenchmarkTimingState *state = (BenchmarkTimingState *)userData;
    ChunkTiming *entry;
    uint64_t now_ns;
    uint64_t elapsed_ns_since_start;

    (void)chunkIndex;
    (void)totalChunks;
    (void)dataWin;

    if (state == NULL || chunkName == NULL || strcmp(chunkName, "DONE") == 0) {
        return;
    }

    now_ns = monotonic_ns_now();
    if (state->parse_started_ns == 0) {
        state->parse_started_ns = now_ns;
    }

    elapsed_ns_since_start = now_ns - state->parse_started_ns;
    entry = timing_state_find_or_add(state, chunkName);
    if (entry == NULL) {
        return;
    }

    entry->total_ns += elapsed_ns_since_start;
    if (entry->count == 0 || elapsed_ns_since_start > entry->max_ns) {
        entry->max_ns = elapsed_ns_since_start;
    }
    entry->count += 1;
}

static void aggregate_chunk_timings(BenchmarkTimingState *aggregate, const BenchmarkTimingState *run) {
    size_t i;

    if (aggregate == NULL || run == NULL) {
        return;
    }

    for (i = 0; i < run->chunk_count; ++i) {
        ChunkTiming *entry = timing_state_find_or_add(aggregate, run->chunks[i].name);
        if (entry == NULL) {
            continue;
        }

        entry->total_ns += run->chunks[i].total_ns;
        if (run->chunks[i].max_ns > entry->max_ns) {
            entry->max_ns = run->chunks[i].max_ns;
        }
        entry->count += run->chunks[i].count;
    }
}

static int compare_chunk_timings(const void *lhs, const void *rhs) {
    const ChunkTiming *a = (const ChunkTiming *)lhs;
    const ChunkTiming *b = (const ChunkTiming *)rhs;

    if (a->total_ns < b->total_ns) {
        return 1;
    }
    if (a->total_ns > b->total_ns) {
        return -1;
    }
    return strcmp(a->name, b->name);
}

static void print_chunk_breakdown(const BenchmarkTimingState *state, const char *mode_name, int iterations) {
    ChunkTiming *sorted;
    size_t i;
    size_t limit;

    if (state == NULL || state->chunk_count == 0) {
        printf("chunk timing: no data captured\n");
        return;
    }

    sorted = (ChunkTiming *)malloc(state->chunk_count * sizeof(*sorted));
    if (sorted == NULL) {
        fprintf(stderr, "error: out of memory while sorting chunk timings\n");
        return;
    }

    memcpy(sorted, state->chunks, state->chunk_count * sizeof(*sorted));
    qsort(sorted, state->chunk_count, sizeof(*sorted), compare_chunk_timings);

    limit = state->chunk_count < 10 ? state->chunk_count : 10;
    printf("top chunks by aggregated parse time (%s, %d iterations):\n", mode_name, iterations);
    for (i = 0; i < limit; ++i) {
        double total_ms = (double)sorted[i].total_ns / 1e6;
        double avg_ms = (double)sorted[i].total_ns / (double)iterations / 1e6;
        double max_ms = (double)sorted[i].max_ns / 1e6;
        printf("  %-8s total=%.3f ms avg=%.3f ms max=%.3f ms count=%zu\n",
               sorted[i].name,
               total_ms,
               avg_ms,
               max_ms,
               sorted[i].count);
    }

    free(sorted);
}

static int parse_once_and_measure(const char *path, uint64_t *elapsed_ns_out, BenchmarkTimingState *run_state) {
    DataWin dw = {0};
    DataWinParserOptions options = {0};
    struct timespec start;
    struct timespec end;
    int rc;

    if (path == NULL || *path == '\0') {
        fprintf(stderr, "error: file path is empty\n");
        return -1;
    }

    if (elapsed_ns_out == NULL || run_state == NULL) {
        fprintf(stderr, "error: benchmark state is required\n");
        return -1;
    }

    timing_state_reset(run_state);

    rc = DataWin_loadFile(&dw, path);
    if (rc != 0) {
        fprintf(stderr, "error: failed to load file '%s' (errno=%d)\n", path, errno);
        return -1;
    }

    DataWin_initParserOptions(&options);
    options.progressCallback = benchmark_progress_callback;
    options.progressCallbackUserData = run_state;
    run_state->parse_started_ns = monotonic_ns_now();

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        fprintf(stderr, "error: clock_gettime(start) failed\n");
        DataWin_free(&dw);
        return -1;
    }

    rc = DataWin_parseWithOptions(&dw, &options);
    if (rc != 0) {
        fprintf(stderr, "error: DataWin_parseWithOptions failed for '%s'\n", path);
        DataWin_free(&dw);
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        fprintf(stderr, "error: clock_gettime(end) failed\n");
        DataWin_free(&dw);
        return -1;
    }

    *elapsed_ns_out = elapsed_ns(&start, &end);
    DataWin_free(&dw);
    return 0;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "usage: %s <path-to-data.win> [iterations] [warmup-runs]\n", prog);
    fprintf(stderr, "example: %s ../private/DELTARUNE/chapter2_windows/data.win 10 1\n", prog);
}

int main(int argc, char **argv) {
    const char *path;
    int iterations = 10;
    int warmup_runs = 1;
    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    uint64_t total_ns = 0;
    uint64_t sample_ns;
    BenchmarkTimingState run_state = {0};
    BenchmarkTimingState aggregate = {0};
    int i;
    const char *mode_name =
#ifdef MULTITHREAD
        "multithreaded";
#else
        "single-threaded";
#endif

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    path = argv[1];
    if (argc >= 3) {
        iterations = atoi(argv[2]);
    }
    if (argc >= 4) {
        warmup_runs = atoi(argv[3]);
    }

    if (iterations <= 0) {
        fprintf(stderr, "error: iterations must be > 0\n");
        return 1;
    }
    if (warmup_runs < 0) {
        fprintf(stderr, "error: warmup-runs must be >= 0\n");
        return 1;
    }

    for (i = 0; i < warmup_runs; ++i) {
        if (parse_once_and_measure(path, &sample_ns, &run_state) != 0) {
            return 1;
        }
    }

    for (i = 0; i < iterations; ++i) {
        if (parse_once_and_measure(path, &sample_ns, &run_state) != 0) {
            return 1;
        }

        aggregate_chunk_timings(&aggregate, &run_state);

        if (sample_ns < min_ns) {
            min_ns = sample_ns;
        }
        if (sample_ns > max_ns) {
            max_ns = sample_ns;
        }
        total_ns += sample_ns;
    }

    printf("mode=%s file=%s iterations=%d warmup=%d total_ms=%.3f avg_ms=%.3f min_ms=%.3f max_ms=%.3f\n",
           mode_name,
           path,
           iterations,
           warmup_runs,
           (double)total_ns / 1e6,
           (double)total_ns / (double)iterations / 1e6,
           (double)min_ns / 1e6,
           (double)max_ns / 1e6);

    print_chunk_breakdown(&aggregate, mode_name, iterations);

    if (aggregate.chunks != NULL) {
        free(aggregate.chunks);
    }
    if (run_state.chunks != NULL) {
        free(run_state.chunks);
    }

    return 0;
}
