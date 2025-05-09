#include <string.h>
#include <stdlib.h>

#include "editor.h"
#include "config.h"
#include "utils.h"
#include "rows.h"

//Row opperations
int editorRowCxToRx(const erow *row, int cx)
{
  int rx = 0;
  int j;

  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t') {
      rx += (EASY_C_TAB_STOP - 1) - (rx % EASY_C_TAB_STOP);
    }
    rx++;
  }
  return rx;
}

void editorUpdateRow(erow *row, const int mod)
{
  int tabs = 0;
  int j;

  if (!row || !row->chars) {
    return;
  }

  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      tabs++;
    }
  }

  free(row->render);
  size_t alloc_size = row->size + tabs * (EASY_C_TAB_STOP - 1) + 1;
  row->render = xmalloc(alloc_size ? alloc_size : 1, 0);

  int idx = 0;

  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';

      while (idx % EASY_C_TAB_STOP != 0) {
        row->render[idx++] = ' ';
      }
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = 0;
  row->rsize = idx;

  if (mod)
  {
    E->modified++;
  }
}

void editorAppendRow(int at, const char *s, size_t len)
{
  if (at < 0 || at > E->numRows) {
    return;
  }

  E->row = xrealloc(E->row, sizeof(erow) * (E->numRows + 1));
  memmove(&E->row[at + 1], &E->row[at], sizeof(erow) * (E->numRows - at));

  E->row[at].capacity = len + 1;
  E->row[at].size = len;
  E->row[at].chars = xmalloc(len + 1, 0);

  if (len > 0) {
    memcpy(E->row[at].chars, s, len);
  }

  E->row[at].chars[len] = '\0';

  E->row[at].rsize = 0;
  E->row[at].render = NULL;
  editorUpdateRow(&E->row[at], 0);

  E->numRows++;
  editorIndexRows();
}

void editorDelRow(int at)
{
  if (at < 0 || at >= E->numRows) {
    return;
  }

  erow *row = &E->row[at];
  free(row->chars);
  free(row->render);

  memmove(&E->row[at], &E->row[at + 1], sizeof(erow) * (E->numRows - at - 1));
  E->numRows--;
  editorIndexRows();
}

void editorRowInsertChar(erow *row, int at, int c)
{
  if (at < 0 || at > row->size) {
    at = row->size;
  }

  if (row->size + 1 >= row->capacity) {
    int newCapacity = row->capacity + ROW_GROWTH_CHUNK;
    if (newCapacity < row->size + 2) {
      newCapacity = row->size + 2;
    }
    row->chars = xrealloc(row->chars, newCapacity);
    row->capacity = newCapacity;
  }
  if (at == row->size) {
    row->chars[at] = c;
    row->chars[at + 1] = '\0';
  } else {
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->chars[at] = c;
  }
  row->size++;
  editorUpdateRow(row, 1);
}

void editorRowDelChar(erow *row, int at)
{
  if (at < 0 || at > row->size) {
    return;
  }

  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;

  editorUpdateRow(row, 1);
}

void editorRowAppendString(erow *row, const char *s, size_t len)
{
  row->chars = xrealloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row, 1);
}

char *editorRowsToString(int *bufLen)
{
  char *buf = NULL, *p;
  int totalLen = 0;
  int j;

  for (j = 0; j < E->numRows; j++) {
    totalLen += E->row[j].size + 1;
  }

  *bufLen = totalLen;
  totalLen++;

  buf = xmalloc(totalLen, 1);
  p = buf;

  for (j = 0; j < E->numRows; j++) {
    memcpy(p, E->row[j].chars, E->row[j].size);
    p += E->row[j].size;
    *p = '\n';
    p++;
  }

  *p = '\0';
  return buf;

}


