#include "arena.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct Arena {
    char *base;
    size_t offset;
    size_t capacity;
};

Arena *arena_create(size_t capacity) {
    Arena *a = malloc(sizeof(Arena));
    if (!a) return NULL;

    a->base = mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a->base == MAP_FAILED) {
        free(a);
        return NULL;
    }

    a->offset = 0;
    a->capacity = capacity;
    return a;
}

void *arena_alloc(Arena *a, size_t size, size_t align) {
    if (!a || size == 0) return NULL;

    uintptr_t curr = (uintptr_t)a->base + a->offset;
    uintptr_t aligned = (curr + align - 1) & ~(align - 1UL);
    size_t padding = aligned - curr;

    if (a->offset + padding + size > a->capacity) {
        return NULL;  
    }

    void *ptr = a->base + a->offset + padding;
    a->offset += padding + size;
    return ptr;
}

void arena_reset(Arena *a) {
    if (a) a->offset = 0;
}

void arena_destroy(Arena *a) {
    if (a) {
        if (a->base) munmap(a->base, a->capacity);
        free(a);
    }
}