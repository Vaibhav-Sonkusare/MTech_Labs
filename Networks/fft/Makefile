# ============================================================
#   Fast UDP File Transfer Project — Makefile
# ============================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -std=c11 -O2 -g
INCLUDES:= -Iinclude

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests

# ============================================================
#   Collect all source files dynamically
# ============================================================
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

# Convert to object files in build/
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Executables
SENDER  := $(BIN_DIR)/sender
RECEIVER:= $(BIN_DIR)/receiver
TEST_EXEC := $(BIN_DIR)/test_protocol

# ============================================================
#   Build Everything
# ============================================================
all: dirs $(SENDER) $(RECEIVER)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# ============================================================
#   Compile all .c → .o
# ============================================================
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================================================
#   Build Executables Separately
# ============================================================

# All object files except sender.o
RECEIVER_OBJS := $(filter-out $(BUILD_DIR)/sender.o, $(OBJS))

# All object files except receiver.o
SENDER_OBJS := $(filter-out $(BUILD_DIR)/receiver.o, $(OBJS))

# Sender executable
$(SENDER): $(SENDER_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# Receiver executable
$(RECEIVER): $(RECEIVER_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# ============================================================
#   Tests
# ============================================================
TEST_OBJS := $(filter-out $(BUILD_DIR)/sender.o $(BUILD_DIR)/receiver.o, $(OBJS))

test: $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_DIR)/test_protocol.c $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# ============================================================
#   Clean
# ============================================================
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean test dirs
