#ifndef ROWS_H
#define ROWS_H

#include <stddef.h>

typedef struct erow
{
  int index;
  int capacity;
  int size;
  int rsize;
  char *chars;
  char *render;
} erow;

int editorRowCxToRx(const erow *row, int cx);
void editorUpdateRow(erow *row, const int mod);
void editorAppendRow(int at, const char *s, size_t len);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowDelChar(erow *row, int at);
void editorRowAppendString(erow *row, const char *s, size_t len);
char *editorRowsToString(int *bufLen);

#endif
