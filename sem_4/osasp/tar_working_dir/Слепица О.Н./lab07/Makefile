CC = gcc
DEBUG_FLAGS = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -D_GNU_SOURCE
RELEASE_FLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror -D_GNU_SOURCE

ifeq ($(MODE),debug)
    CFLAGS = $(DEBUG_FLAGS)
    BUILD_DIR = build/debug
else
    CFLAGS = $(RELEASE_FLAGS)
    BUILD_DIR = build/release
endif

.PHONY: all clean debug release

all: build_dir start

build_dir:
	mkdir -p $(BUILD_DIR)

start: $(BUILD_DIR)/main

$(BUILD_DIR)/main: $(BUILD_DIR)/main.o $(BUILD_DIR)/file_operations.o $(BUILD_DIR)/user_interface.o
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/file_operations.o: src/file_operations.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/user_interface.o: src/user_interface.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build *.o

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release
