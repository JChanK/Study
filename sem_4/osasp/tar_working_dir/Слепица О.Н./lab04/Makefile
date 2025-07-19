CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic -Werror
SRC_DIR = src
BIN_DIR = bin
HEADER = $(SRC_DIR)/header.h

.PHONY: all clean dirs

all: dirs $(BIN_DIR)/main $(BIN_DIR)/producer $(BIN_DIR)/consumer

dirs:
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/main: $(SRC_DIR)/main.c $(HEADER)
	$(CC) $(CFLAGS) $< -o $@ -lrt -pthread

$(BIN_DIR)/producer: $(SRC_DIR)/producer.c $(HEADER)
	$(CC) $(CFLAGS) $< -o $@ -lrt -pthread

$(BIN_DIR)/consumer: $(SRC_DIR)/consumer.c $(HEADER)
	$(CC) $(CFLAGS) $< -o $@ -lrt -pthread

clean:
	rm -rf $(BIN_DIR)
