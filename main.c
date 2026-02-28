#include "arena.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define N          10000000
#define SZ         64
#define ALIGNMENT  8

int main(int argc, char **argv) {
    size_t capacity; 

    if (argc > 1) {
        capacity = atoll(argv[1]);
    } else {
        capacity = 1024 * 1024 * 1024;
}
    
    Arena *arena = arena_create(capacity);
    if (arena == NULL) {
        fprintf(stderr, "Arena creation failed\n");
        return 1;
    }
    printf("Benchmark: %zu allocations of %zu bytes (arena capacity: %zu bytes)\n\n", N, SZ, capacity);

    // Bump allocator loop
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
    printf("Bump arena   : %.3f s  →  ~%.0f ns per alloc     (successful: %zu)\n",
           bump_sec, bump_sec * 1e9 / bump_count, bump_count);

    arena_reset(arena);

    // malloc + free comparison
    printf("\n");
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
    printf("malloc + free: %.3f s  →  ~%.0f ns per alloc+free (successful: %zu)\n",
           malloc_sec, malloc_sec * 1e9 / malloc_count, malloc_count);

    arena_destroy(arena);
    return 0;
}