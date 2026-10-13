# Makefile for the ls re-implementation (BSDSF24A025-OS-A02)
#
# The source lives in src/, intermediate object files are placed in obj/
# and the final executable is linked into bin/ (named `ls`).

CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu11

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

TARGET  := $(BIN_DIR)/ls
SRC     := $(SRC_DIR)/ls-v1.0.0.c
OBJ     := $(OBJ_DIR)/ls-v1.0.0.o

.PHONY: all clean

all: $(TARGET)

# Link the final executable into bin/
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Compile the single translation unit into obj/
$(OBJ): $(SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

# Create the output directories on demand
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -f $(OBJ) $(TARGET)
