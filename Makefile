CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude

BIN_DIR = bin

SERVER = $(BIN_DIR)/server
CLIENT = $(BIN_DIR)/client

SERVER_SRCS = \
	src/main.c \
	src/ipc.c \
	src/crypto.c \
	src/executor.c \
	src/utils.c

CLIENT_SRCS = \
    client/client.c \
    src/ipc.c

all: $(BIN_DIR) $(SERVER) $(CLIENT)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER): $(SERVER_SRCS)
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $(SERVER)

$(CLIENT): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o $(CLIENT)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
