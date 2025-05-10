#define _POSIX_C_SOURCE 200809L

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

void pushRowUndoStepSelection(void)
{
  int startY = E->cy < E->sel_sy ? E->cy : E->sel_sy;
  int endY   = E->cy > E->sel_sy ? E->cy : E->sel_sy;
  int numRows = endY - startY + 1;

  if (numRows < 1) {
    return;
  }

  RowState *changes = xmalloc(sizeof(RowState) * numRows, 1);
  for (int i = 0; i < numRows; i++) {
    int idx = startY + i;
    changes[i].row_index = idx;
    if (idx >= 0 && idx < E->numRows) {
      changes[i].oldContent = strdup(E->row[idx].chars);
    } else {
      changes[i].oldContent = NULL;
    }
    changes[i].newContent = NULL;
  }

  pushRowUndoStep(&undoRowStack, changes, numRows, E->cx, E->cy);
  clearRowUndoStack(&redoRowStack);

  for (int i = 0; i < numRows; i++) {
    if (changes[i].oldContent) {
      free(changes[i].oldContent);
    }
  }
  free(changes);
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
  free(step);
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
        free(row->chars);
        row->size = step->state[i].oldSize;
        row->capacity = step->state[i].oldCapacity;
        row->chars = xmalloc(row->capacity, 0);
        memcpy(row->chars, step->state[i].oldContent, row->size + 1);
        row->chars[row->size] = '\0';
        editorUpdateRow(row, 1);
    }
    E->cx = step->cx;
    E->cy = step->cy;

    step->next = redoRowStack->top;
    redoRowStack->top = step;
}

void performRowRedo(UndoRowStack *redoRowStack, UndoRowStack *undoRowStack, editorConfig *E)
{
  UndoStep *step = popRowUndoStep(redoRowStack);

  if (!step) {
    return;
  }

  for (int i = 0; i < step->changeCount; i++) {
    int idx = step->state[i].row_index;
    if (idx < 0 || idx >= E->numRows) {
      continue;
    }
    erow *row = &E->row[idx];
    free(row->chars);
    row->size = step->state[i].newSize;
    row->capacity = step->state[i].newCapacity;
    row->chars = xmalloc(row->capacity, 0);
    memcpy(row->chars, step->state[i].newContent, row->size + 1);
    row->chars[row->size] = '\0';
    editorUpdateRow(row, 1);
  }
  E->cx = step->cx;
  E->cy = step->cy;

  step->next = undoRowStack->top;
  undoRowStack->top = step;
}
