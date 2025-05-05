# Compiler and flags
CC = clang
CFLAGS = -std=c99 -Wall -Werror

# Project files
SRC = text_editor.c utils.c terminal.c rows.c input.c commands.c editor_commands.c
OBJ = $(SRC:.c=.o)
TARGET = text_editor

# Default rule - Build the project
all: $(TARGET)

# Linking the object files to create the final executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule to compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean the build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets to avoid conflicts with file names
.PHONY: all clean
