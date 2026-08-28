CC = gcc
CFLAGS = -Wall -Wextra -pthread -Iinclude
SRC = src/main.c src/udp_listener.c src/tcp_listener.c src/nodos.c src/logger.c src/config_loader.c
BIN = build/servidor

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

clean:
	rm -rf build/*

run: all
	./$(BIN)

.PHONY: all clean run