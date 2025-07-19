# Компилятор и флаги
CC = gcc
CFLAGS_DEBUG = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CFLAGS_RELEASE = -std=c11 -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

# Директории
SRC_DIR = src
BUILD_DEBUG_DIR = build/debug
BUILD_RELEASE_DIR = build/release

# Исходные файлы
SRC_PARENT = $(SRC_DIR)/parent.c
SRC_CHILD = $(SRC_DIR)/child.c

# Объектные файлы
OBJ_PARENT_DEBUG = $(BUILD_DEBUG_DIR)/parent.o
OBJ_CHILD_DEBUG = $(BUILD_DEBUG_DIR)/child.o
OBJ_PARENT_RELEASE = $(BUILD_RELEASE_DIR)/parent.o
OBJ_CHILD_RELEASE = $(BUILD_RELEASE_DIR)/child.o

# Целевые исполняемые файлы (в текущей директории)
TARGET_PARENT = parent
TARGET_CHILD = child

# Правила сборки
all: debug

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET_PARENT) $(TARGET_CHILD)

release: CFLAGS = $(CFLAGS_RELEASE)
release: $(TARGET_PARENT) $(TARGET_CHILD)

# Сборка родительского процесса
$(TARGET_PARENT): $(OBJ_PARENT_DEBUG) $(OBJ_PARENT_RELEASE)
	$(CC) $(CFLAGS) -o $(TARGET_PARENT) $<

# Сборка дочернего процесса
$(TARGET_CHILD): $(OBJ_CHILD_DEBUG) $(OBJ_CHILD_RELEASE)
	$(CC) $(CFLAGS) -o $(TARGET_CHILD) $<

# Компиляция объектных файлов (debug)
$(OBJ_PARENT_DEBUG): $(SRC_PARENT)
	@mkdir -p $(BUILD_DEBUG_DIR)
	$(CC) $(CFLAGS) -c -o $(OBJ_PARENT_DEBUG) $(SRC_PARENT)

$(OBJ_CHILD_DEBUG): $(SRC_CHILD)
	@mkdir -p $(BUILD_DEBUG_DIR)
	$(CC) $(CFLAGS) -c -o $(OBJ_CHILD_DEBUG) $(SRC_CHILD)

# Компиляция объектных файлов (release)
$(OBJ_PARENT_RELEASE): $(SRC_PARENT)
	@mkdir -p $(BUILD_RELEASE_DIR)
	$(CC) $(CFLAGS) -c -o $(OBJ_PARENT_RELEASE) $(SRC_PARENT)

$(OBJ_CHILD_RELEASE): $(SRC_CHILD)
	@mkdir -p $(BUILD_RELEASE_DIR)
	$(CC) $(CFLAGS) -c -o $(OBJ_CHILD_RELEASE) $(SRC_CHILD)

# Очистка
clean:
	rm -rf $(BUILD_DEBUG_DIR) $(BUILD_RELEASE_DIR) $(TARGET_PARENT) $(TARGET_CHILD)

# Псевдоцели
.PHONY: all debug release clean
