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

# Manual page and installation paths
MAN_SRC := $(SRC_DIR)/../man/man1/ls.1
PREFIX  ?= /usr/local
BINDIR  := $(DESTDIR)$(PREFIX)/bin
MANDIR  := $(DESTDIR)$(PREFIX)/share/man/man1

.PHONY: all clean install uninstall man

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

# Render the manual page on the terminal (groff/mandoc)
man:
	groff -man -Tascii $(MAN_SRC) | less -R

# Install the program and its manual page system wide.
# The system directories are protected, so run it as:  sudo make install
install: $(TARGET)
	install -d $(BINDIR) $(MANDIR)
	install -m 755 $(TARGET) $(BINDIR)/ls
	install -m 644 $(MAN_SRC) $(MANDIR)/ls.1
	-mandb

# Remove the installed files again
uninstall:
	rm -f $(BINDIR)/ls $(MANDIR)/ls.1
