#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Error handling
void explodeProgram (const char *string);

//Memory management
void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);

// Append buffer operations
struct abuf {
    char *b;
    int len;
    int capacity;
};

void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

#endif
