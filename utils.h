#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Error handling
void explodeProgram (const char *string);

//Memory management
void *xmalloc (size_t size, _Bool zero);
void *xrealloc (void *ptr, size_t size);

// Arena memory handling
typedef struct
{
  char *base;
  size_t used;
  size_t capacity;
} Arena;

Arena *arena_create(size_t capacity, _Bool zero);
void *arena_alloc(Arena *a, size_t size);
void arena_reset(Arena *a);
void arena_free(Arena *a);

// Append buffer operations
struct abuf {
    char *b;
    int len;
    int capacity;
};

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

#endif
