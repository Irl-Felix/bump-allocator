#include "arena.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/resource.h>

#define N          ((size_t)10000000)
#define SZ         ((size_t)64)
#define ALIGNMENT  ((size_t)8)

int main(int argc, char **argv) {
    size_t capacity;
    if (argc > 1) {
        capacity = atoll(argv[1]);
    } else {
        capacity = 1024 * 1024 * 1024;  // 1GB
    }

    Arena *arena = arena_create(capacity);
    if (arena == NULL) {
        fprintf(stderr, "Arena creation failed\n");
        return 1;
    }

    printf("Benchmark: %zu allocations of %zu bytes (arena capacity: %zu bytes)\n\n",
           N, SZ, capacity);

    // Bump allocator loop
    struct rusage usage_start, usage_end;
    getrusage(RUSAGE_SELF, &usage_start);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t bump_count = 0;
    for (size_t i = 0; i < N; i++) {
        void *p = arena_alloc(arena, SZ, ALIGNMENT);
        if (p == NULL) {
            fprintf(stderr, "Bump alloc failed at %zu (successful: %zu)\n", i, bump_count);
            break;
        }
        *(volatile char *)p = 42;   // touch to force real memory access
        bump_count++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double bump_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Bump arena   : %.3f s → ~%.0f ns per alloc     (successful: %zu)\n",
           bump_sec, bump_sec * 1e9 / bump_count, bump_count);

    getrusage(RUSAGE_SELF, &usage_end);
    long bump_rss_delta = usage_end.ru_maxrss - usage_start.ru_maxrss;
    printf("Bump RSS delta (fragmentation proxy): %ld KB\n", bump_rss_delta);

    arena_reset(arena);

    // malloc + free comparison 
    printf("\n");

    getrusage(RUSAGE_SELF, &usage_start);

    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t malloc_count = 0;
    for (size_t i = 0; i < N; i++) {
        void *p = malloc(SZ);
        if (p == NULL) {
            fprintf(stderr, "malloc failed at %zu\n", i);
            break;
        }
        *(volatile char *)p = 42;
        free(p);
        malloc_count++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double malloc_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("malloc + free: %.3f s → ~%.0f ns per alloc+free (successful: %zu)\n",
           malloc_sec, malloc_sec * 1e9 / malloc_count, malloc_count);

    getrusage(RUSAGE_SELF, &usage_end);
    long malloc_rss_delta = usage_end.ru_maxrss - usage_start.ru_maxrss;
    printf("malloc RSS delta (fragmentation proxy): %ld KB\n", malloc_rss_delta);

    arena_destroy(arena);
    return 0;
}