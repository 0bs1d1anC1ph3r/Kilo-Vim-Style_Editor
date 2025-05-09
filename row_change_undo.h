#ifndef ROW_CHANGE_UNDO_H
#define ROW_CHANGE_UNDO_H

typedef struct
{
  int row_index;
  char *oldContent;
  char *newContent;
} RowState;

typedef struct UndoStackNode
{
  RowState *state;
  int changeCount;
  int cursorX, cursorY;
  struct UndoStackNode *next;
} UndoStackNode;

typedef struct UndoStack
{
  UndoStackNode *top;
} UndoStack;

extern UndoStack redoStack;
extern UndoStack undoStack;

#endif
