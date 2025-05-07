#include <string.h>

#include "history.h"
#include "utils.h"

history H;

editorConfig *copyEditorConfig(editorConfig *old) {
  if (old == NULL || old->row == NULL || old->numRows < 0) {
    explodeProgram("Invalid editorConfig object passed to copyEditorConfig");
  }
  editorConfig *new = xmalloc(sizeof(editorConfig));
  new->cx = old->cx;
  new->cy = old->cy;
  new->rx = old->rx;
  new->rowoff = old->rowoff;
  new->coloff = old->coloff;
  new->screenRows = old->screenRows;
  new->screenCols = old->screenCols;
  new->numRows = old->numRows;
  new->row = xmalloc(sizeof(erow) * (old->numRows));

  for (int i = 0; i < new->numRows; i++) {
    new->row[i].size = old->row[i].size;
    new->row[i].rsize = old->row[i].rsize;
    if (old->row[i].chars == NULL || old->row[i].render == NULL) {
      explodeProgram("Invalid row data in editorConfig");
    }
    new->row[i].chars = xmalloc(old->row[i].size + 1);
    memcpy(new->row[i].chars, old->row[i].chars, old->row[i].size + 1);
    new->row[i].render = xmalloc(old->row[i].rsize + 1);
    memcpy(new->row[i].render, old->row[i].render, old->row[i].rsize + 1);
  }
  return new;
}

struct editorConfig *historyPush(struct editorConfig *snapshot)
{
  if (snapshot == NULL){
    explodeProgram("Null snapshot passed to historyPush");
  }

  editorConfig *new_E = copyEditorConfig(snapshot);

  if (new_E == NULL) {
    explodeProgram("Error creating snapshot");
  }

  snapshot->redo = new_E;
  new_E->undo = snapshot;
  return new_E;
}

struct editorConfig *historyUndo(struct editorConfig *E)
{
  if (E->undo) {
    return E->undo;
  }
  return E;
}

struct editorConfig *historyRedo(struct editorConfig *E)
{
  if (E->redo) {
    return E->redo;
  }
  return E;
}
