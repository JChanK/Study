CC = gcc
DEBUG_FLAGS = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
RELEASE_FLAGS = -std=c11 -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

# Add pthread to both compile and link flags
PTHREAD_FLAGS = -pthread

ifeq ($(MODE),debug)
    CFLAGS = $(DEBUG_FLAGS) $(PTHREAD_FLAGS)
    BUILD_DIR = build/debug
else
    CFLAGS = $(RELEASE_FLAGS) $(PTHREAD_FLAGS)
    BUILD_DIR = build/release
endif

LDFLAGS = $(PTHREAD_FLAGS)
BIN_DIR = bin
TARGET = $(BIN_DIR)/main

.PHONY: all clean debug release

all: build_dirs $(TARGET)

build_dirs:
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET): $(BUILD_DIR)/main.o $(BUILD_DIR)/queue.o $(BUILD_DIR)/utils.o
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/main.o: src/main.c src/queue.h src/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/queue.o: src/queue.c src/queue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: src/utils.c src/utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release
