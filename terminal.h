#ifndef TERMINAL_H
#define TERMINAL_H

void disableRawMode(void);
void enableRawMode(void);
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int *rows, int *cols);

#endif
