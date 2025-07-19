CC = gcc
CFLAGS_DEBUG = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CFLAGS_RELEASE = -std=c11 -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable
DEBUG_DIR = build/debug
RELEASE_DIR = build/release
ifeq ($(MODE),release)
    CFLAGS = $(CFLAGS_RELEASE)
    BUILD_DIR = $(RELEASE_DIR)
else
    CFLAGS = $(CFLAGS_DEBUG)
    BUILD_DIR = $(DEBUG_DIR)
endif

.PHONY: all clean

all: parent child

parent: $(BUILD_DIR)/parent.o $(BUILD_DIR)/parent_ops.o $(BUILD_DIR)/child_ops.o
	$(CC) $^ -o $@

child: $(BUILD_DIR)/child.o $(BUILD_DIR)/child_ops.o $(BUILD_DIR)/parent_ops.o
	$(CC) $^ -o $@

$(BUILD_DIR)/parent.o: src/parent.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/child.o: src/child.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/parent_ops.o: src/parent_ops.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/child_ops.o: src/child_ops.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(RELEASE_DIR)/*.o parent child $(DEBUG_DIR)/*.o
