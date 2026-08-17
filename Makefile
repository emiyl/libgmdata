CC = gcc
AR = /usr/bin/ar
RANLIB = /usr/bin/ranlib
CPPFLAGS = -Isrc -Iinclude
CFLAGS = -Wall -Wextra -std=c11

PREFIX ?= /usr/local
INSTALL_LIB_DIR ?= $(PREFIX)/lib
INSTALL_INC_DIR ?= $(PREFIX)/include
INSTALL_BIN_DIR ?= $(PREFIX)/bin

BUILD_DIR = build
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

LIB_SRCS = $(shell find src -type f -name '*.c' ! -name 'main.c')
LIB_OBJS = $(LIB_SRCS:%.c=$(BUILD_DIR)/%.o)
LIB_TARGET = $(LIB_DIR)/libgmdata.a

CLI_SRC = tools/gmdataparser.c
CLI_TARGET = $(BIN_DIR)/gmdataparser

all: $(LIB_TARGET) $(CLI_TARGET)

lib: $(LIB_TARGET)

install: $(LIB_TARGET) $(CLI_TARGET)
	install -d "$(INSTALL_BIN_DIR)" "$(INSTALL_LIB_DIR)" "$(INSTALL_INC_DIR)"
	install -m 755 "$(CLI_TARGET)" "$(INSTALL_BIN_DIR)/gmdataparser"
	install -m 644 "$(LIB_TARGET)" "$(INSTALL_LIB_DIR)/libgmdata.a"
	install -m 644 include/*.h "$(INSTALL_INC_DIR)/"

$(LIB_TARGET): $(LIB_OBJS)
	mkdir -p $(LIB_DIR)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(CLI_TARGET): $(CLI_SRC) $(LIB_TARGET)
	mkdir -p $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(CLI_SRC) $(LIB_TARGET)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all lib install clean