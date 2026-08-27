CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread -g
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, build/%.o, $(SRC))
TARGET = build/server

all: dirs $(TARGET)

dirs:
	mkdir -p build

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build/*

.PHONY: all dirs clean
