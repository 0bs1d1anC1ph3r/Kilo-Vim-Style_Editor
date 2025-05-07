#include <unistd.h>
#include <errno.h>

#include "utils.h"
#include "rows.h"
#include "editor.h"

//Low-level terminal input
int editorReadKey(void)
{
    int nread; //Number of bytes read
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            explodeProgram("read");
        }
    }

    if (c == '\x1b') {
        char seq[3];

         if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }

        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
                return '\x1b';
        }

        if (seq[0] == '[') {
          if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(STDIN_FILENO, &seq[2], 1) != 1) {
              return '\x1b';
            }
            if (seq[2] == '~') {
              switch (seq[1]) {
                case '1':
                  return HOME_KEY;
                case '3':
                  return DEL_KEY;
                case '4':
                  return END_KEY;
                case '5':
                  return PAGE_UP;
                case '6':
                  return PAGE_DOWN;
                case '7':
                  return HOME_KEY;
                case '8':
                  return END_KEY;
              }
            }
          } else {
              switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
          }
        } else if (seq[0] == 'O') {
          switch(seq[1]) {
            case 'H': return HOME_KEY;
            case 'F': return END_KEY;
          }
        }
        return '\x1b';
    } else {
        return c;
    }
}

void editorMoveCursor(int key)
{
  erow *row;
  if (E->cy >= E->numRows) {
    row = NULL;
  } else {
    row = &E->row[E->cy];
  }

  switch (key)
  {
    case 'h':
    case ARROW_LEFT:
    case 'A':
      if (E->cx != 0) {
        E->cx--;
      } else if (E->cy > 0) {
        E->cy--;
        E->cx = E->row[E->cy].size;
      }
      break;
    case 'l':
    case ARROW_RIGHT:
    case 'D':
      if (row && E->cx < row->size) {
        E->cx++;
      } else if (row && E->cx == row->size) {
        E->cy++;
        E->cx = 0;
      }
      break;
    case 'k':
    case ARROW_UP:
    case 'W':
      if (E->cy != 0) {
        E->cy--;
      }
      break;
    case 'j':
    case ARROW_DOWN:
    case 'S':
      if (E->cy < E->numRows - 1) {
        E->cy++;
      }
      break;
    }

  if (E->cy >= E->numRows) {
    row = NULL;
  } else {
    row = &E->row[E->cy];
  }

  int rowLen;
  if (row) {
    rowLen = row->size;
  } else {
    rowLen = 0;
  }

  if (E->cx > rowLen) {
    E->cx = rowLen;
  }
}

void editorProcessKeypress(void)
{
    int c = editorReadKey();

    if (mode == MODE_INSERT) {
      if (c == 27) {
        mode = MODE_NORMAL;
      } else {
        switch (c) {
          case ARROW_LEFT:
          case ARROW_UP:
          case ARROW_DOWN:
          case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
          case BACKSPACE:
          case CTRL_KEY('h'):
          case DEL_KEY: {
            if (c == DEL_KEY) {
              editorMoveCursor(ARROW_RIGHT);
            }
            editorDelChar();
            break;
          }
          case '\r':
            editorInsertNewLine();
            break;
          default: {
            editorInsertChar(c);
            break;
          }
        }
      }
    } else if (mode == MODE_NORMAL) {
      switch (c)
      {
        case END_KEY:
          E->cx = 0;
          break;
        case HOME_KEY:
          if (E->cy < E->numRows) {
            E->cx = E->row[E->cy].size;
          }
          break;
        case PAGE_UP:
        case PAGE_DOWN:
          {
            if (c == PAGE_UP) {
              E->cy = E->rowoff;
            } else if (c == PAGE_DOWN) {
              E->cy = E->rowoff + E->screenRows - 1;
              if (E->cy > E->numRows) {
                E->cy = E->numRows;
              }
            }
            int times = E->screenRows;
            while (times--) {
              editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
          }
        case 'i':
          mode = MODE_INSERT;
          break;
        case ':':
          editorCommandMode();
          break;
        case 'h':
        case 'l':
        case 'j':
        case 'k':
          editorMoveCursor(c);
          break;
        case 'v':
          if (!E->selecting) {
            E->selecting = 1;
            E->sel_sx = E->cx;
            E->sel_sy = E->cy;
          } else {
            E->selecting = 0;
            E->cy = E->sel_sy;
            E->cx = E->sel_sx;
          }
          break;
        case 27:
          if (E->selecting) {
            E->selecting = 0;
          }
          break;
        case 'y':
          if (E->selecting) {
            editorBufferSelection();
            editorCopyToClipboard(E->selectBuf, E->selectBufLen);
          }
          break;
        case CTRL_KEY('y'):
          if (E->selecting) {
            editorBufferSelection();
            editorCopyToClipboard(E->selectBuf, E->selectBufLen);

            E->selecting = 0;
            E->cy = E->sel_sy;
            E->cx = E->sel_sx;
          }
          break;
        case 'd':
          if (E->selecting) {
            editorDelSelected();
            E->selecting = 0;
          }
          break;
        case 'x':
          if (E->selecting) {
            editorBufferSelection();
            editorCopyToClipboard(E->selectBuf, E->selectBufLen);
            editorDelSelected();

            E->selecting = 0;
          }
          break;
        case ARROW_RIGHT:
        case ARROW_DOWN:
        case ARROW_UP:
        case ARROW_LEFT:
          if (E->selecting) {
            editorMoveCursor(c);
          }
          break;
      }
    }
}

