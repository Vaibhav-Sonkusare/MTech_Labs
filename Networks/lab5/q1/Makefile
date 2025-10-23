CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../../include
SRC_DIR = ../../src

SERVER_FILES = ./server.c
SERVER_BIN = ./server

CLIENT1_FILES = ./sender_client.c
CLIENT1_BIN = ./sender_client

CLIENT2_FILES = ./receiver_client.c
CLIENT2_BIN = ./receiver_client

.DEFAULT_GOAL := help

.PHONY: all clean help server client run_server run_client

server:
# 	echo "$(CC) $(CFLAGS) $(SERVER_FILES) $(SRC_DIR)/network_utils.c -o $(SERVER_BIN)"
	$(CC) $(CFLAGS) $(SERVER_FILES) $(SRC_DIR)/network_utils_v2.c -o $(SERVER_BIN)

client:
	$(CC) $(CFLAGS) $(CLIENT1_FILES) $(SRC_DIR)/network_utils_v2.c -o $(CLIENT1_BIN)
	$(CC) $(CFLAGS) $(CLIENT2_FILES) $(SRC_DIR)/network_utils_v2.c -o $(CLIENT2_BIN)

all: server client

run_server: server
	./$(SERVER_BIN)

run_client: client
	./$(CLIENT_BIN)

clean:
	rm $(SERVER_BIN)
	rm $(CLIENT1_BIN)
	rm $(CLIENT2_BIN)

help:
	@echo "Makefile Usage:"
	@echo "  make server      - Compile server"
	@echo "  make client      - Compile client"
	@echo "  make all         - Combile server and client"
	@echo "  make run_server  - Compile and run server"
	@echo "  make run_client  - Compile and run client"
	@echo "  make clean       - Clean client and server binaries"