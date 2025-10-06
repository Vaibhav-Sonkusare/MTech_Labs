CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../../include
SRC_DIR = ../../src

# Helper function to read filenames
SERVER_FILE = $(shell cat _server_name.txt 2>/dev/null)
CLIENT_FILE = $(shell cat _client_name.txt 2>/dev/null)

SERVER_BIN = $(basename $(SERVER_FILE))
CLIENT_BIN = $(basename $(CLIENT_FILE))

.DEFAULT_GOAL := help

.PHONY: all clean help server client run build

# Build any given file with network_utils.c
# Usage: make build FILE=<filename.c>
build:
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE=<filename.c>"; \
		exit 1; \
	else \
		echo "Compiling $(FILE) with network_utils.c"; \
		$(CC) $(CFLAGS) $(FILE) $(SRC_DIR)/network_utils.c -o $(basename $(FILE))
	fi

# Run a given file (after building it)
# Usage: make run FILE=<filename.c>
run: build
	./$(basename $(FILE))

server:
	@if [ -f _server_name.txt ]; then \
		echo "Compiling $(SERVER_FILE) -> $(SERVER_BIN)"; \
		$(CC) $(CFLAGS) $(SERVER_FILE) $(SRC_DIR)/network_utils.c -o $(SERVER_BIN); \
	else \
		echo "Error: _server_name.txt not found!"; \
		exit 1; \
	fi

run_server: server
	echo "Running: $(SERVER_BIN)"
	./$(SERVER_BIN)

client:
	@if [ -f _client_name.txt ]; then \
		echo "Compiling $(CLIENT_FILE) -> $(CLIENT_BIN)"; \
		$(CC) $(CFLAGS) $(CLIENT_FILE) $(SRC_DIR)/network_utils.c -o $(CLIENT_BIN); \
	else \
		echo "Error: _client_name.txt not found!"; \
		exit 1; \
	fi

run_client: client
	echo "Running: $(CLIENT_BIN)"
	./$(CLIENT_BIN)

all: server client

clean:
	@if [ -f _server_name.txt ]; then \
		echo "Removing $(SERVER_BIN)"; \
		rm -f $(SERVER_BIN); \
	fi
	@if [ -f _client_name.txt ]; then \
		echo "Removing $(CLIENT_BIN)"; \
		rm -f $(CLIENT_BIN); \
	fi
	rm -f *.o *.out *.exe

help:
	@echo "Makefile Usage:"
	@echo "  make build FILE=<filename.c>   - Compile the specified C file with network_utils.c"
	@echo "  make run FILE=<filename.c>     - Compile and run the specified C file"
	@echo "  make server                    - Compile the server program specified in _server_name.txt"
	@echo "  make client                    - Compile the client program specified in _client_name.txt"
	@echo "  make all                       - Compile both server and client programs"
	@echo "  make clean                     - Remove compiled binaries and temporary files"
	@echo "  make help                      - Display this help message"