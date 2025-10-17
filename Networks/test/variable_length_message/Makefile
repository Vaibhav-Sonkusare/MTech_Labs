CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../../include
SRC_DIR = ../../src

SERVER_FILES = ./server.c
SERVER_BIN = ./server

CLIENT_FILES = ./client.c
CLIENT_BIN = ./client

.DEFAULT_GOAL := help

.PHONY: all clean help server client run_server run_client

server:
# 	echo "$(CC) $(CFLAGS) $(SERVER_FILES) $(SRC_DIR)/network_utils.c -o $(SERVER_BIN)"
	$(CC) $(CFLAGS) $(SERVER_FILES) $(SRC_DIR)/network_utils_v2.c -o $(SERVER_BIN)

client:
	$(CC) $(CFLAGS) $(CLIENT_FILES) $(SRC_DIR)/network_utils_v2.c -o $(CLIENT_BIN)

all: server client

run_server: server
	./$(SERVER_BIN)

run_client: client
	./$(CLIENT_BIN)

clean:
	rm $(SERVER_BIN)
	rm $(CLIENT_BIN)

help:
	@echo "Makefile Usage:"
	@echo "  make server      - Compile server"
	@echo "  make client      - Compile client"
	@echo "  make all         - Combile server and client"
	@echo "  make run_server  - Compile and run server"
	@echo "  make run_client  - Compile and run client"
	@echo "  make clean       - Clean client and server binaries"