CC = gcc
DEBUG_FLAGS = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -D_GNU_SOURCE
RELEASE_FLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror -D_GNU_SOURCE
LDFLAGS = -pthread

ifeq ($(MODE),debug)
    CFLAGS = $(DEBUG_FLAGS)
    BUILD_DIR = build/debug
else
    CFLAGS = $(RELEASE_FLAGS)
    BUILD_DIR = build/release
endif

.PHONY: all clean

all: build_dir sort_index view generator

build_dir:
	mkdir -p $(BUILD_DIR)

sort_index: $(BUILD_DIR)/sort_index

view: $(BUILD_DIR)/view

generator: $(BUILD_DIR)/generator

$(BUILD_DIR)/sort_index: $(BUILD_DIR)/sort_index.o $(BUILD_DIR)/func.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/view: $(BUILD_DIR)/read.o $(BUILD_DIR)/func.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/generator: $(BUILD_DIR)/generator.o $(BUILD_DIR)/func.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/sort_index.o: src/sort_index.c src/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/read.o: src/read.c src/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/generator.o: src/generator.c src/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/func.o: src/func.c src/common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

debug: 
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all
