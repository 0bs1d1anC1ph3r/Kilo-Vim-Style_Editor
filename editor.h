#ifndef EDITOR_H
#define EDITOR_H

#include <time.h>
#include <termios.h>

#include "rows.h"

// Constants
#define CTRL_KEY(k) ((k) & 0x1f) // Macro for CTRL key combinations
#define EASY_C_VERSION "0.01"
#define EASY_C_TAB_STOP 8
#define ROW_GROWTH_CHUNK 32

// Editor modes
typedef enum editorMode {
    MODE_INSERT,
    MODE_NORMAL,
    MODE_VISUAL
} editorMode;

extern int mode;

typedef struct editorConfig
{
    int cx, cy; // Cursor position
    int rx; // Render x
    int rowoff;
    int coloff;
    int screenRows;
    int screenCols;
    int numRows;
    erow *row;
    char *filename;
    char statusMsg[80];
    time_t statusMsgTime;
    int modified;
    _Bool selecting;
    int sel_sx, sel_sy;
    char *selectBuf;
    int selectBufLen;
    _Bool newFile;
    struct termios orig_termios;
} editorConfig;

extern editorConfig *E;

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000, //All values after this increment by 1, which is cool, I didn't know that was a thing
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

// File I/O
void editorOpen(const char *filename);

// Editor operations
void undoTypeRedoUndo(_Bool undo);
void editorIndexRows(void);
void editorInsertChar(int c);
void editorDelChar(void);
void editorInsertNewLine(void);
void editorDelSelected(void);

// Clipboard
void editorCopyToClipboard(const char *text, size_t len);
void editorPasteClipboard(void);
void editorBufferSelection(void);

// Screen rendering
void editorSetStatusMessage(const char *fmt, ...);

// Command mode
void editorCommandMode(void);
void editorExecuteCommand(const char *cmd);

#endif
