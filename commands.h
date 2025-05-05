#ifndef COMMANDS_H
#define COMMANDS_H

typedef void (*CommandHandler)(_Bool forceFlag, const char *args);

typedef struct
{
  const char *name;
  CommandHandler handler;
} Command;

void registerCommand(const char *name, CommandHandler handler);
void executeCommand(const char *cmd);

void editorSave(_Bool forceWrite, const char *args);
void editorQuit(_Bool forceQuit, const char *args);
void editorSaveQuit(_Bool forceWrite, const char *args);
void editorEditFile(_Bool forceEdit, const char *args);
#endif

