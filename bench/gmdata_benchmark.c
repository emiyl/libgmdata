#include "src/gmdata.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t elapsed_ns(const struct timespec *start, const struct timespec *end) {
    uint64_t start_ns = (uint64_t)start->tv_sec * 1000000000ULL + (uint64_t)start->tv_nsec;
    uint64_t end_ns = (uint64_t)end->tv_sec * 1000000000ULL + (uint64_t)end->tv_nsec;
    if (end_ns < start_ns) {
        return 0;
    }
    return end_ns - start_ns;
}

static int parse_once_and_measure(const char *path, uint64_t *elapsed_ns_out) {
    DataWin dw = {0};
    struct timespec start;
    struct timespec end;
    int rc;

    if (path == NULL || *path == '\0') {
        fprintf(stderr, "error: file path is empty\n");
        return -1;
    }

    rc = DataWin_loadFile(&dw, path);
    if (rc != 0) {
        fprintf(stderr, "error: failed to load file '%s' (errno=%d)\n", path, errno);
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        fprintf(stderr, "error: clock_gettime(start) failed\n");
        DataWin_free(&dw);
        return -1;
    }

    rc = DataWin_parse(&dw);
    if (rc != 0) {
        fprintf(stderr, "error: DataWin_parse failed for '%s'\n", path);
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
        if (parse_once_and_measure(path, &sample_ns) != 0) {
            return 1;
        }
    }

    for (i = 0; i < iterations; ++i) {
        if (parse_once_and_measure(path, &sample_ns) != 0) {
            return 1;
        }

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

    return 0;
}
