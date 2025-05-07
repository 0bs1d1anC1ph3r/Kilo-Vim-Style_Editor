#ifndef UNDO_H
#define UNDO_H

#include "editor.h"

typedef struct history
{
  editorConfig *buffer[5];
  int size;
} history;

editorConfig *historyPush(struct editorConfig *E);
editorConfig *historyUndo(struct editorConfig *E);
editorConfig *historyRedo(struct editorConfig *E);

#endif
