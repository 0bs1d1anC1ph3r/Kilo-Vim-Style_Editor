#include <stdlib.h>
#include <string.h>

#include "linear_undo.h"
#include "editor.h"
#include "utils.h"

#define UNDO_STACK_LIMIT 100

static UndoState *copyEditorState(const editorConfig *E, Arena *a)
{
  UndoState *state = arena_alloc(a, sizeof(UndoState));
  state->numRows = E->numRows;
  state->cx = E->cx;
  state->cy = E->cy;
  state->row = arena_alloc(a, sizeof(erow) * (size_t)state->numRows);
  for (int i = 0; i < state->numRows; i++) {
    const erow *source = &E->row[i];
    erow *destination = &state->row[i];

    destination->size = source->size;
    destination->capacity = source->capacity;
    destination->rsize = source->rsize;
    destination->render = arena_alloc(a, (size_t)source->rsize + 1);
    memcpy(destination->render, source->render, source->rsize + 1);
    destination->chars = arena_alloc(a, (size_t)source->size + 1);
    memcpy(destination->chars, source->chars, (size_t)source->size + 1);
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
    memcpy(E->row[i].render, state->row[i].render, state->row[i].rsize + 1);
    E->row[i].chars = xmalloc(state->row[i].size + 1, 1);
    memcpy(E->row[i].chars, state->row[i].chars, state->row[i].size + 1);
  }
}

void pushUndoState(UndoStack *stack, const editorConfig *E)
{
  UndoStackNode *node = xmalloc(sizeof(UndoStackNode), 1);
  node->arena = arena_create(4096 * 10, 1);
  node->state = copyEditorState(E, node->arena);
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

static UndoStackNode *popUndoNode(UndoStack *stack)
{
  if (!stack->top) {
    return NULL;
  }
  UndoStackNode *node = stack->top;
  stack->top = node->next;
  return node;
}

static void freeUndoNode(UndoStackNode *node)
{
  arena_free(node->arena);
  free(node);
}

void clearUndoStack(UndoStack *stack)
{
  UndoStackNode *node = stack->top;
  while (node) {
    UndoStackNode *next = node->next;
    freeUndoNode(node);
    node = next;
  }
  stack->top = NULL;
}

void performUndo(UndoStack *undoStack, UndoStack *redoStack, editorConfig *E) {
  UndoStackNode *node = popUndoNode(undoStack);
  if (!node) return;
  pushUndoState(redoStack, E);
  restoreEditorState(E, node->state);
  freeUndoNode(node);
}

void performRedo(UndoStack *redoStack, UndoStack *undoStack, editorConfig *E) {
  UndoStackNode *node = popUndoNode(redoStack);
  if (!node) return;
  pushUndoState(undoStack, E);
  restoreEditorState(E, node->state);
  freeUndoNode(node);
}
