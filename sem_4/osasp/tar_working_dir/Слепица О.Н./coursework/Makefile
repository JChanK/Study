CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -g -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lncursesw -lm -lblkid

# Директории
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Имя исполняемого файла
TARGET = $(BIN_DIR)/fs_analyzer

# Поиск всех .c файлов в директории src
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
# Соответствующие .o файлы в директории obj
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Заголовочные файлы
HEADERS = $(wildcard $(SRC_DIR)/*.h)

# Создание директорий
DIRS = $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean run help

all: $(DIRS) $(TARGET)

# Создание директорий
$(DIRS):
	mkdir -p $@

# Компиляция исполняемого файла
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Компиляция объектных файлов
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Запуск программы
run: all
	sudo $(TARGET)

# Инструкция по использованию
help:
	@echo "Доступные команды:"
	@echo "  make       - Собрать проект"
	@echo "  make clean - Удалить скомпилированные файлы"
	@echo "  make run   - Запустить программу (с sudo для доступа к устройствам)"
	@echo "  make help  - Показать эту справку"
