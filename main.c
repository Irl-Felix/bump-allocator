#include "arena.h"
#include <stdio.h>
#include <time.h>

#define N 10000000
#define SZ 64
#define ALIGNMENT 8

int main(void) {
    Arena *arena = arena_create(64 * 1024 * 1024);
    if (!arena) {
        fprintf(stderr, "Arena creation failed\n");
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < N; i++) {
        void *p = arena_alloc(arena, SZ, ALIGNMENT);
        if (!p) {
            fprintf(stderr, "Alloc failed at %zu\n", i);
            break;
        }
        *(volatile char *)p = 42;  // Tooch memory
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Bump allocator: %.3f s → ~%.0f ns/alloc\n", elapsed, elapsed * 1e9 / N);

    arena_reset(arena);
    arena_destroy(arena);
    return 0;
}