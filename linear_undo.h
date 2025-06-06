#ifndef LINEAR_UNDO_H
#define LINEAR_UNDO_H

#include "rows.h"
#include "editor.h"
#include "utils.h"

typedef struct UndoState
{
  int numRows;
  erow *row;
  int cx, cy;
} UndoState;

typedef struct UndoStackNode
{
  UndoState *state;
  struct UndoStackNode *next;
  Arena *arena;
} UndoStackNode;

typedef struct UndoStack
{
  UndoStackNode *top;
} UndoStack;

extern UndoStack redoStack;
extern UndoStack undoStack;

void pushUndoState(UndoStack *stack, const editorConfig *E);
void clearUndoStack(UndoStack *stack);
void performUndo(UndoStack *undoStack, UndoStack *redoStack, editorConfig *E);
void performRedo(UndoStack *redoStack, UndoStack *undoStack, editorConfig *E);

#endif
