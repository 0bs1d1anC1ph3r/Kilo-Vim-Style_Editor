#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "row_change_undo.h"
#include "config.h"
#include "editor.h"

void pushRowUndoStep(UndoRowStack *stack, RowState *state, int changeCount, int cx, int cy)
{
  UndoStep *step = xmalloc(sizeof(UndoStep) + 1, 1);
  step->arena = arena_create(ARENA_MEM_ALLOC_NUM * 10, 1);
  step->state = arena_alloc(step->arena, sizeof(RowState) * changeCount);
  step->changeCount = changeCount;
  step->cx = cx;
  step->cy = cy;
  step->next = stack->top;
  stack->top = step;

  for (int i = 0; i < changeCount; i++) {
    step->state[i].row_index = state[i].row_index;
    if(state[i].oldContent) {
      size_t len = strlen(state[i].oldContent) + 1;
      step->state[i].oldContent = arena_alloc(step->arena, len);
      memcpy(step->state[i].oldContent, state[i].oldContent, len);
    } else {
      step->state[i].oldContent = NULL;
    }
    if (state[i].newContent) {
      size_t len = strlen(state[i].newContent) + 1;
      step->state[i].newContent = arena_alloc(step->arena, len);
      memcpy(step->state[i].newContent, state[i].newContent, len);
    } else {
      step->state[i].newContent = NULL;
    }
  }
}

UndoStep *popRowUndoStep(UndoRowStack *stack)
{
  if (!stack->top) {
    return NULL;
  }
  UndoStep *step = stack->top;
  stack->top = step->next;
  return step;
}

static void freeUndoStep(UndoStep *step)
{
  if (!step) {
    return;
  }
  if (step->arena) {
    arena_free(step->arena);
  }
}

void clearRowUndoStack(UndoRowStack *stack)
{
  UndoStep *step = stack->top;
  while (step) {
    UndoStep *next = step->next;
    freeUndoStep(step);
    step = next;
  }
  stack->top = NULL;
}

void performRowUndo(UndoRowStack *undoRowStack, UndoRowStack *redoRowStack, editorConfig *E)
{
  UndoStep *step = popRowUndoStep(undoRowStack);

  if (!step) {
    return;
  }

  for (int i = 0; i < step->changeCount; i++) {
    int idx = step->state[i].row_index;
    if (idx < 0 || idx >= E->numRows) {
      continue;
    }
    erow *row = &E->row[idx];
    const char *content = step->state[i].oldContent;
    free(row->chars);
    row->size = strlen(content);
    row->chars = xmalloc(row->size + 1, 0);
    memcpy(row->chars, content, row->size + 1);
    editorUpdateRow(row, 1);
  }
  E->cx = step->cx;
  E->cy = step->cy;

  step->next = redoRowStack->top;
  redoRowStack->top = step;
}
