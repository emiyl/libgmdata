CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Isrc

BUILD_DIR = build
SRCS = $(shell find src -type f -name '*.c')
OBJ = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET = gmdataparser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean