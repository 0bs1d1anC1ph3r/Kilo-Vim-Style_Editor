#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

//Error handling
void explodeProgram(const char *string)
{
    perror(string);
    exit(1);
}

// Memory handling
void *xmalloc(size_t size)
{
  void *p = malloc(size);
  if (!p) {
    explodeProgram("malloc");
  }

  return p;
}

// Rallocating memory handling
void *xrealloc(void *ptr, size_t size)
{
  void *p = realloc(ptr, size);
  if (!p) {
    explodeProgram("realloc");
  }

  return p;
}

// Append buffer
struct abuf
{
    char *b;
    int len;
    int capacity;
};

// Append to buffer
void abAppend(struct abuf *ab, const char *s, int len)
{
    if (ab -> len + len > ab -> capacity) {
        int newCapacity = ab -> capacity;

        if (newCapacity == 0) {
            newCapacity = 32;
        } else {
            newCapacity = newCapacity * 2;
        }

        while (newCapacity < ab -> len + len) {
            newCapacity *= 2;
        }

    char *newBuf = xrealloc(ab -> b, newCapacity);

    ab -> b = newBuf;
    ab -> capacity = newCapacity;

    }
    memcpy(&ab -> b[ab -> len], s, len);
    ab -> len += len;
}

// Free appended buffer
void abFree(struct abuf *ab)
{
    free(ab -> b);
    ab -> b = NULL;
    ab -> len = 0;
    ab -> capacity = 0;
}


