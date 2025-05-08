#include <stdlib.h>
#include <string.h>

#include "linear_undo.h"
#include "editor.h"
#include "utils.h"

#define UNDO_STACK_LIMIT 100

static UndoState *copyEditorState(const editorConfig *E)
{
  UndoState *state = xmalloc(sizeof(UndoState), 1);
  state->numRows = E->numRows;
  state->cx = E->cx;
  state->cy = E->cy;
  state->row = xmalloc(sizeof(erow) * state->numRows, 1);
  for (int i = 0; i < state->numRows; i++) {
    state->row[i].size = E->row[i].size;
    state->row[i].capacity = E->row[i].capacity;
    state->row[i].rsize = E->row[i].rsize;
    state->row[i].render = xmalloc(E->row[i].rsize + 1, 1);
    memcpy(state->row[i].render, E->row[i].render, E->row[i].rsize);
    state->row[i].chars = xmalloc(E->row[i].size + 1, 1);
    memcpy(state->row[i].chars, E->row[i].chars, E->row[i].size + 1);
  }
  return state;
}

static void restoreEditorState(editorConfig *E, const UndoState *state)
{
  for (int i = 0; i < E->numRows; i++) {
    free(E->row[i].chars);
    free(E->row[i].render);
  }
  free(E->row);

  E->numRows = state->numRows;
  E->cx = state->cx;
  E->cy = state->cy;
  E->row = xmalloc(sizeof(erow) * E->numRows, 1);
  for (int i = 0; i < E->numRows; i++) {
    E->row[i].size = state->row[i].size;
    E->row[i].capacity = state->row[i].capacity;
    E->row[i].rsize = state->row[i].rsize;
    E->row[i].render = xmalloc(state->row[i].rsize + 1, 1);
    memcpy(E->row[i].render, state->row[i].render, state->row[i].rsize);
    E->row[i].chars = xmalloc(state->row[i].size + 1, 1);
    memcpy(E->row[i].chars, state->row[i].chars, state->row[i].size + 1);
  }
}

void pushUndoState(UndoStack *stack, const editorConfig *E)
{
  UndoStackNode *node = xmalloc(sizeof(UndoStackNode), 1);
  node->state = copyEditorState(E);
  node->next = stack->top;
  stack->top = node;

  int count = 0;
  UndoStackNode *current = stack->top;
  UndoStackNode *prev = NULL;
  while (current) {
    count++;
    if (count > UNDO_STACK_LIMIT) {
      if (prev) prev->next = NULL;
      clearUndoStack(&(UndoStack){ .top = current });
      break;
    }
    prev = current;
    current = current->next;
  }
}

UndoState *popUndoState(UndoStack *stack)
{
  if (!stack->top) {
    return NULL;
  }
  UndoStackNode *node = stack->top;
  UndoState *state = node->state;
  stack->top = node->next;
  free(node);
  return state;
}

void freeUndoState(UndoState *state)
{
  for (int i = 0; i < state->numRows; i++) {
    free(state->row[i].chars);
    free(state->row[i].render);
  }
  free(state->row);
  free(state);
}

void clearUndoStack(UndoStack *stack)
{
  UndoStackNode *node = stack->top;
  while (node) {
    UndoStackNode *next = node->next;
    freeUndoState(node->state);
    free(node);
    node = next;
  }
  stack->top = NULL;
}

void performUndo(UndoStack *undoStack, UndoStack *redoStack, editorConfig *E) {
  UndoState *state = popUndoState(undoStack);
  if (!state) return;
  pushUndoState(redoStack, E);
  restoreEditorState(E, state);
  freeUndoState(state);
}

void performRedo(UndoStack *redoStack, UndoStack *undoStack, editorConfig *E) {
  UndoState *state = popUndoState(redoStack);
  if (!state) return;
  pushUndoState(undoStack, E);
  restoreEditorState(E, state);
  freeUndoState(state);
}
