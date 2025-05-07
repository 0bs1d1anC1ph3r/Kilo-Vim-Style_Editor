//Includes
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h> // Atexit
#include <unistd.h> // Read, STDIN_FILENO
#include <termios.h> // Basically all of the termios/ tc.../ raw.c...
#include <sys/ioctl.h> // Terminal Input/Output Control (get the window size)
#include <sys/types.h> // For ssize_t
#include <string.h> // Append buffer use
#include <time.h> // For timestamp
#include <stdarg.h> // For va_list
#include <limits.h> // For PATH_MAX
#include <errno.h> // For errors
#include <fcntl.h> // For File I/O flags

#include "rows.h"
#include "editor.h"
#include "config.h"
#include "utils.h"
#include "history.h"

void editorQuit(_Bool forceQuit, const char *args, struct editorConfig *E)
{
  if (args != NULL) {
    editorSetStatusMessage("Invalid number of arguments");
    return;
  }

  if (E->modified && forceQuit == 0) {
    editorSetStatusMessage("You have unsaved modifications. To exit without saving, use \":!q\"");
    return;
  }

    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
}

void editorSave(_Bool forceWrite, const char *args, struct editorConfig *E)
{
  if (args != NULL && args[0] != '\0') {
    editorSetStatusMessage("Invalid number of arguments");
    return;
  }

  _Bool backupCreated = 0;

  int len;
  char *buf = editorRowsToString(&len);

  char tmpFile[PATH_MAX];
  snprintf(tmpFile, sizeof(tmpFile), "%s.tmp", E->filename);

  if (forceWrite == 0 && BACKUP_CREATE == 1 && access(E->filename, F_OK) == 0) {
    char bakFile[PATH_MAX];
    snprintf(bakFile, sizeof(bakFile), "%s.bak", E->filename);

    if (rename(E->filename, bakFile) != 0) {
      editorSetStatusMessage("Error saving: Unable to write backup - %s. To save without a backup, use the command: \"!w\"", strerror(errno));
      free(buf);
      return;
    } else {
      backupCreated = 1;
    }
  }

  int fd = open(tmpFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd < 0) {
    editorSetStatusMessage("Error saving: Unable to write temp file - %s", strerror(errno));
    unlink(tmpFile);
    free(buf);
    return;
  }

  if (write(fd, buf, len) != len) {
    editorSetStatusMessage("Error saving: Write failed - %s", strerror(errno));
    close(fd);
    unlink(tmpFile);
    free(buf);
    return;
  }

  fsync(fd);
  close(fd);

  if (rename(tmpFile, E->filename) != 0) {
    editorSetStatusMessage("Error saving: Failed to rename temp file - %s", strerror(errno));
    unlink(tmpFile);
    free(buf);
    return;
  }

  free(buf);
  E->modified = 0;
  E->newFile = 0;

  char fullPath[PATH_MAX];

  editorSetStatusMessage("%d bytes written at %s. %s", len,
      realpath(E->filename, fullPath) ? fullPath : E->filename,
      backupCreated ? "(Backup Created)" : "(No Backup)");

}

void editorSaveQuit(_Bool forceWrite, const char *args, struct editorConfig *E)
{
  editorSave(forceWrite, args, E);
  editorQuit(0, NULL, E);
}

void editorEditFile(_Bool forceEdit, const char *args, struct editorConfig *E)
{
  if (!args) {
    editorSetStatusMessage("Invalid number of arguments");
    return;
  }

  while (*args == ' ') {
    args++;
  }

  if (E->modified && !forceEdit) {
    editorSetStatusMessage("You have unsaved modifications, to edit another file without saving, use: \":!e\"");
    return;
  }

  char *filename = strdup(args);

  if (!filename) {
    explodeProgram("Failed to allocate memory");
  }

  editorOpen(filename);
}

void editorUndo(_Bool force, const char *args, struct editorConfig *E)
{
  if (args != NULL) {
    editorSetStatusMessage("Invalid number of arguments");
    return;
  }

  E = historyUndo(E);
}

void editorRedo(_Bool force, const char *args, struct editorConfig *E)
{
  if (args != NULL) {
    editorSetStatusMessage("Invalid number of arguments");
    return;
  }
  E = historyRedo(E);
}
