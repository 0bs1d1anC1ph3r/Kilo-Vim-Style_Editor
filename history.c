#include <string.h>

#include "history.h"
#include "utils.h"

history H;

editorConfig *copyEditorConfig(editorConfig *old)
{
  editorConfig *new = xmalloc(sizeof(editorConfig));

  new -> cx = old ->cx;
  new -> cy = old -> cy;
  new -> rx = old -> rx;
  new -> rowoff = old -> rowoff;
  new -> coloff = old -> coloff;
  new -> screenRows = old -> screenRows;
  new -> screenCols = old -> screenCols;

  new -> row = xmalloc(sizeof(erow) * (old -> numRows));
  for (int i = 0; i < new -> numRows; i++) {
    new -> row[i].size = old -> row[i].size;
    new -> row[i].rsize = old -> row[i].rsize;

    new -> row[i].chars = xmalloc (old -> row[i].size);
    memcpy(new -> row[i].chars, old -> row[i].chars, old -> row[i].size);

    new -> row[i].render = xmalloc (old -> row[i].rsize);
    memcpy(new -> row[i].render, old -> row[i].render, old -> row[i].rsize);
  }
  return new;
}

struct editorConfig *historyPush(struct editorConfig *snapshot)
{
  editorConfig *new_E = copyEditorConfig(snapshot);
  snapshot -> redo = new_E;
  new_E -> undo = snapshot;
  return new_E;
}

struct editorConfig *historyUndo(struct editorConfig *E)
{
  if (E -> undo) {
    return E -> undo;
  }
  return E;
}

struct editorConfig *historyRedo(struct editorConfig *E)
{
  if (E -> redo) {
    return E -> redo;
  }
  return E;
}
