#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

Arena *arena_create(size_t capacity);
void *arena_alloc(Arena *arena, size_t size, size_t align);
void arena_reset(Arena *arena);
void arena_destroy(Arena *arena);

#endif