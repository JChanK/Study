CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11 -D_GNU_SOURCE -Wno-format-truncation
SRC_DIR = src
BUILD_DIR = build

# Цели по умолчанию
all: $(BUILD_DIR)/myserver $(BUILD_DIR)/myclient

# Сборка сервера
$(BUILD_DIR)/myserver: $(SRC_DIR)/server.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Сборка клиента
$(BUILD_DIR)/myclient: $(SRC_DIR)/client.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -rf $(BUILD_DIR)

# Создание директории src если не существует
init:
	@mkdir -p $(SRC_DIR)

# Запуск тестов
test: all
	@echo "Для тестирования запустите:"
	@echo "Терминал 1: ./$(BUILD_DIR)/myserver /tmp 8080"
	@echo "Терминал 2: ./$(BUILD_DIR)/myclient localhost 8080"

.PHONY: all clean init test
