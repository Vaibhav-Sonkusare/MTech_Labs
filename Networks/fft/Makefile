# ============================================================
#   Fast UDP File Transfer Project — Makefile
# ============================================================

# ---- Compiler and Flags ----
CC      := gcc
CFLAGS  := -Wall -Wextra -Werror -std=c11 -O2 -g
INCLUDES:= -Iinclude
LDFLAGS := 

# ---- Directories ----
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin
TEST_DIR := tests

# ---- Source Files ----
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# ---- Executables ----
SENDER  := $(BIN_DIR)/sender
RECEIVER:= $(BIN_DIR)/receiver
TEST_EXEC := $(BIN_DIR)/test_protocol

# ============================================================
#   Default Target
# ============================================================
all: dirs $(SENDER) $(RECEIVER)

# Make necessary folders
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# ============================================================
#   Build Rules
# ============================================================

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ---- Sender Executable ----
$(SENDER): $(BUILD_DIR)/sender.o $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# ---- Receiver Executable ----
$(RECEIVER): $(BUILD_DIR)/receiver.o $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# ============================================================
#   Tests
# ============================================================

test: $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_DIR)/test_protocol.c $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

# ============================================================
#   Utilities
# ============================================================

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run_sender:
	./$(SENDER)

run_receiver:
	./$(RECEIVER)

.PHONY: all clean test dirs run_sender run_receiver
