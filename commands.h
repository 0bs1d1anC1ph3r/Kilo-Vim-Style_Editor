#ifndef COMMANDS_H
#define COMMANDS_H

typedef void (*CommandHandler)(_Bool forceFlag, const char *args, struct editorConfig *E);

typedef struct
{
  const char *name;
  CommandHandler handler;
} Command;

void registerCommand(const char *name, CommandHandler handler);
void executeCommand(const char *cmd);

void editorSave(_Bool forceWrite, const char *args, struct editorConfig *E);
void editorQuit(_Bool forceQuit, const char *args, struct editorConfig *E);
void editorSaveQuit(_Bool forceWrite, const char *args, struct editorConfig *E);
void editorEditFile(_Bool forceEdit, const char *args, struct editorConfig *E);
void editorUndoCommand(_Bool force, const char *args, struct editorConfig *E);
void editorRedoCommand(_Bool force, const char *args, struct editorConfig *E);
#endif

