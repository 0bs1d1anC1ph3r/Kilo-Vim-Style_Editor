//Includes
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h> // Atexit
#include <unistd.h> // Read, STDIN_FILENO
#include <termios.h> // Basically all of the termios/ tc.../ raw.c...
#include <sys/ioctl.h> // Terminal Input/Output Control (get the window size)
#include <sys/types.h> // For ssize_t
#include <string.h> // Append buffer use
#include <time.h> // For timestamp
#include <stdarg.h> // For va_list
#include <ctype.h> // For iscntrl
#include <limits.h> // For PATH_MAX
#include <fcntl.h> // For File I/O flags

#include "rows.h"
#include "editor.h"
#include "terminal.h"
#include "config.h"
#include "utils.h"
#include "input.h"
#include "commands.h"
#include "linear_undo.h"

// Initialization
editorConfig *E = NULL;
UndoStack undoStack = {NULL};
UndoStack redoStack = {NULL};

int mode = MODE_NORMAL;

static void initCommands()
{
  registerCommand("w", editorSave);
  registerCommand("!w", editorSave);
  registerCommand("q", editorQuit);
  registerCommand("!q", editorQuit);
  registerCommand("e", editorEditFile);
  registerCommand("!e", editorEditFile);
  registerCommand("wq", editorSaveQuit);
  registerCommand("!wq", editorSaveQuit);
}

// Cleanup
static void freeEditorConfig(struct editorConfig *E)
{
  if (E == NULL) return;

  if (E->filename) {
    free(E->filename);
    E->filename = NULL;
  }
  if (E->selectBuf) {
    free(E->selectBuf);
    E->selectBuf = NULL;
  }

  if (E->row) {
    for (int i = 0; i < E->numRows; i++) {
      free(E->row[i].chars);
      free(E->row[i].render);
    }
    free(E->row);
  }
    free(E);
}

static void cleanupWrapper(void) {
  disableRawMode();
  clearUndoStack(&undoStack);
  clearUndoStack(&redoStack);
  freeEditorConfig(E);
}

//Data
void editorCopyToClipboard(const char *text, size_t len)
{
  const char *display = getenv("WAYLAND_DISPLAY");
  FILE *pipe = NULL;

  if (display) {
    pipe = popen("wl-copy", "w");
  } else {
    pipe = popen("xclip -selection clipboard", "w");
  }

  if (!pipe) {
    editorSetStatusMessage("ERROR COPYING TO CLIPBOARD -- Only Wayland and Xorg (through xclip) works currently.");
  }

  fwrite(text, sizeof(char), len, pipe);
  pclose(pipe);
}

//Editor opperations
void editorIndexRows(void)
{
  for (int i = 0; i < E->numRows; i++) {
    E->row[i].index = i;
  }
}

void editorInsertChar(int c)
{
  if (E->cy == E->numRows) {
    editorAppendRow(E->numRows, "", 0);
  }

  editorRowInsertChar(&E->row[E->cy], E->cx, c);
  E->cx++;
}

void editorDelChar(void)
{
  if (E->cy == E->numRows) {
    return;
  }

  if (E->cx == 0 && E->cy == 0) {
    return;
  }

  erow *row = &E->row[E->cy];
  if (E->cx > 0) {
    editorRowDelChar(row, E->cx - 1);
    E->cx--;
  } else {
    E->cx = E->row[E->cy - 1].size;
    editorRowAppendString(&E->row[E->cy - 1], row -> chars, row -> size);
    editorDelRow(E->cy);
    E->cy--;
  }
}

static _Bool editorIsSelected(int fileRow, int fileCol)
{
  if (!E->selecting) {
    return 0;
  }
    int startX = E->sel_sx, startY = E->sel_sy;
    int endX = E->cx, endY = E->cy;

    if (startY > endY || (startY == endY && startX > endX)) {
      int tmpX = startX, tmpY = startY;
            startX = endX;
            startY = endY;
            endX = tmpX;
            endY = tmpY;
          }
    int cx = fileCol, cy = fileRow;

    if (cy > startY && cy < endY) {
      return 1;
    } else if (cy == startY && cy == endY && cx >= startX && cx < endX) {
      return 1;
    } else if (cy == startY && cy < endY && cx >= startX) {
      return 1;
    } else if (cy == endY && cy > startY && cx < endX) {
      return 1;
    }
    return 0;
}

void editorDelSelected(void)
{
  int startY = E->sel_sy;
  int startX = E->sel_sx;
  int endY = E->cy;
  int endX = E->cx;

  if (startY > endY || (startY == endY && startX > endX)) {
    int tmpY = startY;
    startX = endX;
    startY = endY;
    endY = tmpY;
  }
  for (int y = startY; y <= endY && y < E->numRows;) {
    int len = E->row[y].rsize;
    int writePos = 0;

    for (int x = 0; x < len; x++) {
      if (!editorIsSelected(y, x)) {
        E->row[y].render[writePos++] = E->row[y].render[x];
      }
    }
    E->row[y].rsize = writePos;

    if (writePos > 0) {
      E->row[y].render[writePos] = '\0';
      E->row[y].size = editorRowCxToRx(&E->row[y], writePos);
      y++;
    } else {
      editorDelRow(y);
      endY--;
    }
  }
  E->cy = startY;
  E->cx = startX;
  E->modified++;
  editorIndexRows();
}


void editorInsertNewLine(void)
{
  if (E->cx == 0) {
    editorAppendRow(E->cy, "", 0);
  } else {
    erow *row = &E->row[E->cy];
    editorAppendRow(E->cy + 1, &row->chars[E->cx], row->size - E->cx);
    row = &E->row[E->cy];
    row->size = E->cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row, 1);
  }
  E->cy++;
  E->cx = 0;
}

//File I/O
void editorOpen(const char *filename)
{
  clearUndoStack(&undoStack);
  clearUndoStack(&redoStack);

  while (E->numRows > 0) { // Delete all rows from buffer
    editorDelRow(E->numRows - 1);
  }

  E->cx = 0; //Set cursor position to the top left of the file
  E->cy = 0;

  free(E->filename);
  E->filename = strdup(filename); // Filename
  E->modified = 0; // Set file to be unmodified
  E->newFile = 0;

  FILE *fp = fopen(filename, "r");

  if (!fp) {
    fp = fopen(filename, "w"); // Write file

    if (!fp) {
      explodeProgram("fopen");
    }

    fclose(fp);
    fp = fopen(filename, "r"); // Open file in read

    if (!fp) {
      explodeProgram("fopen");
    }

    E->newFile = 1; // Set new file true
  }

  char *line = NULL;
  size_t lineCap = 0;
  ssize_t lineLen;
  while ((lineLen = getline(&line, &lineCap, fp)) != -1) {
    while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r')) {
      lineLen--;
    }
    editorAppendRow(E->numRows, line, lineLen);
  }

  if (E->numRows == 0) {
    editorAppendRow(0, "", 0);
  }

  free(line);
  fclose(fp);
}

//Input
void editorBufferSelection(void)
{
  if (!E || !E->selecting) {
    editorSetStatusMessage("No active selection to copy.");
    return;
  }

  free(E->selectBuf);
  E->selectBuf = NULL;
  E->selectBufLen = 0;

  struct abuf ab = ABUF_INIT;

  for (int y = 0; y < E->numRows; y++) {
    int len = E->row[y].rsize;
    _Bool rowHasSelection = 0;

    for (int x = 0; x < len; x++) {
      if (editorIsSelected(y, x)) {
        abAppend(&ab, &E->row[y].render[x], 1);
        rowHasSelection = 1;
      }
    }
    if (rowHasSelection) {
      abAppend(&ab, "\n", 1);
    }
  }
  E->selectBuf = ab.b;
  E->selectBufLen = ab.len;

  if (!E->selectBuf || E->selectBufLen == 0) {
    editorSetStatusMessage("No selection to copy.");
  }
}

//Output
static void editorScroll(void)
{
  E->rx = 0;
  if (E->cy < E->numRows) {
    E->rx = editorRowCxToRx(&E->row[E->cy], E->cx);
  }

  if (E->cy < E->rowoff) {
    E->rowoff = E->cy;
  }
  if (E->cy >= E->rowoff + E->screenRows) {
    E->rowoff = E->cy - E->screenRows + 1;
  }
  if (E->rx < E->coloff) {
    E->coloff = E->rx;
  }
  if (E->rx >= E->coloff + E->screenCols) {
    E->coloff = E->rx - E->screenCols + 1;
  }
}

static void editorDrawRows(struct abuf *ab)
{
  int y;
  for (y = 0; y < E->screenRows; y++) {
    int fileRow = y + E->rowoff;

    if (fileRow >= E->numRows) {
      if (E->numRows == 0 && y == E->screenRows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
            "Easy C -- version %s", EASY_C_VERSION);
      if (welcomelen > E->screenCols) welcomelen = E->screenCols;
      int padding = (E->screenCols - welcomelen) / 2;

      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }

      while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E->row[fileRow].rsize - E->coloff;
      if (len < 0) {
        len = 0;
      }

      if (len > E->screenCols) {
        len = E->screenCols;
      }

      for (int j = 0; j < len;) {
        int cx = j + E->coloff;
        int cy = fileRow;

        if (editorIsSelected(cy, cx)) {
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &E->row[fileRow].render[j + E->coloff], 1);
          abAppend(ab, "\x1b[m", 3);
          j++;
      } else {
        int start = j;
        while (j < len) {
          cx = j + E->coloff;
          cy = fileRow;

          if (editorIsSelected(cy, cx)) {
            break;
          }
          j++;
        }
        abAppend(ab, &E->row[fileRow].render[start + E->coloff], j - start);
      }
    }
  }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

static void editorDrawStatusBar(struct abuf *ab)
{
  abAppend(ab, "\x1b[7m", 4);
  char status[BAR_CHAR_LIMIT], rstatus[BAR_CHAR_LIMIT];
  const char *modeName = (mode == MODE_NORMAL) ? "-- NORMAL --" : "-- INSERT --";
  const char *newFile = (E->newFile) ? "[ NEW ] " : "";

  int len = snprintf(status, sizeof(status), "%s | %s\"%.20s\" -- %d Lines %s",
      modeName, newFile ,E->filename ? E->filename : "[No Name]", E->numRows,
      E->modified ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d:%d/%d",
      E->row[E->cy].index + 1, E->numRows, E->rx + 1, (E->row[E->cy].rsize) + 1);
  if (len > E->screenCols) {
    len = E->screenCols;
  }

  abAppend(ab, status, len);

  while (len < E->screenCols) {
    if (E->screenCols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

static void editorDrawMessageBar(struct abuf *ab)
{
  abAppend(ab, "\x1b[K", 3);
  int msgLen = strlen(E->statusMsg);

  if (msgLen > E->screenCols) {
    msgLen = E->screenCols;

  }

  if (msgLen > 0) {
    char timebuf[16];
    const struct tm *tm = localtime(&E->statusMsgTime);
    strftime(timebuf, sizeof(timebuf), "[%H:%M:%S] -- ", tm);

    abAppend(ab, timebuf, strlen(timebuf));
    abAppend(ab, E->statusMsg, msgLen);
  }

}

static void editorUpdateCursor(void)
{
  switch (mode)
  {
    case MODE_INSERT:
      write(STDOUT_FILENO, "\x1b[6 q", 5);
      break;
    case MODE_NORMAL:
    case MODE_VISUAL:
      if (E->selecting) {
        write(STDOUT_FILENO, "\x1b[6 q", 5);
      } else {
        write(STDOUT_FILENO,"\x1b[2 q", 5);
        break;
      }
  }
}

static void editorRefreshScreen(void)
{
  editorScroll();
  editorUpdateCursor();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E->cy - E->rowoff) + 1, (E->rx - E->coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E->statusMsg, sizeof(E->statusMsg), fmt, ap);
  va_end(ap);
  E->statusMsgTime = time(NULL);
}

//Command handler
void editorCommandMode(void)
{
  char cmd[BAR_CHAR_LIMIT - 1] = {0};
  int cmdlen = 0;

  editorSetStatusMessage(":");
  editorRefreshScreen();

  while (1) {
    int c = editorReadKey();
    if (c == '\r') {
      cmd[cmdlen] = '\0';
      executeCommand(cmd);
      break;
    } else if (c == 27) {
      editorSetStatusMessage("");
      break;
    } else if (c == 127 || c == '\b') {
      if (cmdlen > 0) {
        cmd[--cmdlen] = '\0';
        editorSetStatusMessage(":%s", cmd);
        editorRefreshScreen();
      }
    }

    else if (!iscntrl(c) && cmdlen < sizeof(cmd) - 1) {
      cmd[cmdlen++] = c;
      cmd[cmdlen] = '\0';
      editorSetStatusMessage(":%s", cmd);
      editorRefreshScreen();
    }
  }
}

//Init
static void initEditor(struct editorConfig *E)
{
  E->cx = 0;
  E->cy = 0;
  E->rx = 0;
  E->rowoff = 0;
  E->coloff = 0;
  E->numRows = 0;
  E->row = NULL;
  E->filename = NULL;
  E->statusMsg[0] = '\0';
  E->statusMsgTime = 0;
  E->modified = 0;
  E->selecting = 0;
  E->sel_sx = 0;
  E->sel_sy = 0;
  E->selectBuf = NULL;
  E->selectBufLen = 0;
  E->newFile = 0;
  if (getWindowSize(&E->screenRows,&E->screenCols) == -2) {
    explodeProgram("getWindowSize");
  }
  E->screenRows -= 2; // Do not forget about this just because it is here

}

int main(int argc, char *argv[])
{
  E = malloc(sizeof(editorConfig));
  if (!E) {
    explodeProgram("Failed to allocate editorConfig");
  }
  memset(E, 0, sizeof(editorConfig));

  initEditor(E);
  enableRawMode();
  atexit(cleanupWrapper);
  initCommands();

  if (argc >= 2) {
    editorOpen(argv[1]);
  } else {
    const char *fileName = "MainMenu.txt";
    editorOpen(fileName);
  }

  editorSetStatusMessage("For a list of keybinds and information, enter the command: \":h\"");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
