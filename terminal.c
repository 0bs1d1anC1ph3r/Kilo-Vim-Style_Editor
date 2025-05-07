#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "editor.h"
#include "utils.h"
#include "terminal.h"

//Terminal
void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E->orig_termios) == -1) {
        explodeProgram("tcsetattr");
    }
    write(STDOUT_FILENO, "\x1b[2 q", 5); //Reset cursor as well
}

void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E->orig_termios) == -1) {
        explodeProgram("tcgetattr");
    }

    struct termios raw = E->orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag &= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        explodeProgram("tcsetattr");
    }
}

//Cursor position
int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }

        if (buf[i] == 'R') {
            break;
        }

        i++;
    }

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }

    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

//Window size
int getWindowSize(int *rows, int *cols)
{
    struct winsize ws; //Window Size

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }

        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
