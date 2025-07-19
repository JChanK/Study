CC = gcc
CFLAGS_DEBUG = -std=c11 -Wextra -g -ggdb -pedantic -W -Wall 
CFLAGS_RELEASE = -std=c11 -Wall -pedantic -W -Wextra -Werror -O2 
TARGET = dirwalk
SRC_DIR = src
BUILD_DIR = build
MODE ?= debug

ifeq ($(MODE), release)
    CFLAGS = $(CFLAGS_RELEASE)
    OBJ_DIR = $(BUILD_DIR)/release
else
    CFLAGS = $(CFLAGS_DEBUG)
    OBJ_DIR = $(BUILD_DIR)/debug
endif

SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/dirwalk.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)  s

clean:
	rm -f $(BUILD_DIR)/release/*.o $(BUILD_DIR)/debug/*.o $(TARGET)
