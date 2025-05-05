#include <string.h>

#include "editor.h"
#include "commands.h"

#define MAX_COMMANDS 100

static Command commandRegistry[MAX_COMMANDS];
static int commandCount = 0;

void registerCommand(const char *name, CommandHandler handler)
{
  if (commandCount >= MAX_COMMANDS) {
    editorSetStatusMessage("Command registery is full.");
  } else {
    commandRegistry[commandCount].name = name;
    commandRegistry[commandCount].handler = handler;
    commandCount++;
  }
}

void executeCommand(const char *cmd)
{
  _Bool forceFlag = 0;

  if (cmd[0] == '!') {
    forceFlag = 1;
    cmd++;
  }

  const char *args = strchr(cmd, ' ');
  int commandLen = args ? (args - cmd) : strlen(cmd);

  for (int i = 0; i < commandCount; i++) {
    if (strncmp(cmd, commandRegistry[i].name, commandLen) == 0 &&
        commandRegistry[i].name[commandLen] == '\0') {
      commandRegistry[i].handler(forceFlag, args ? args + 1 : NULL);
      return;
    }
  }
  editorSetStatusMessage("Unsupported command: %.*s", commandLen, cmd);
}
