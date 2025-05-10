#ifndef ROW_CHANGE_UNDO_H
#define ROW_CHANGE_UNDO_H

#include "utils.h"
#include "editor.h"

typedef struct
{
  int row_index;
  char *oldContent;
  char *newContent;
  int oldSize;
  int oldCapacity;
  int newSize;
  int newCapacity;
} RowState;

typedef struct UndoStep
{
  RowState *state;
  int changeCount;
  int cx, cy;
  Arena *arena;
  struct UndoStep *next;
} UndoStep;

typedef struct UndoRowStack
{
  UndoStep*top;
} UndoRowStack;

extern UndoRowStack redoRowStack;
extern UndoRowStack undoRowStack;

void pushRowUndoStep(UndoRowStack *stack, RowState *changes, int change_count, int cx, int cy);
void pushRowUndoStepSelection(void);
UndoStep *popRowUndoStep(UndoRowStack *stack);
void clearRowUndoStack(UndoRowStack *stack);

void performRowUndo(UndoRowStack *undoStack, UndoRowStack *redoStack, editorConfig *E);
void performRowRedo(UndoRowStack *undoStack, UndoRowStack *redoStack, editorConfig *E);
#endif
