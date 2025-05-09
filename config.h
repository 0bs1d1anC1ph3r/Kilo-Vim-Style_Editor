#ifndef CONFIG_H
#define CONFIG_H

#define CTRL_KEY(k) ((k) & 0x1f)
#define EASY_C_VERSION "0.01"
#define ABUF_INIT {NULL, 0, 0}
#define EASY_C_TAB_STOP 8
#define ROW_GROWTH_CHUNK 32
#define BACKUP_CREATE 1
#define BAR_CHAR_LIMIT 80
#define UNDO_REDO_TYPE 1 // 1 = linear everything in buffer, 2 = only changed lines in buffer

#endif
