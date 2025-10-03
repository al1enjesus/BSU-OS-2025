CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -O2
BIN_DIR = bin

all: $(BIN_DIR)/thread_race $(BIN_DIR)/prodcons

$(BIN_DIR)/thread_race: src/thread_race.c
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/thread_race src/thread_race.c

$(BIN_DIR)/prodcons: src/prodcons.c
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/prodcons src/prodcons.c

clean:
	rm -rf $(BIN_DIR)

run: all
	$(BIN_DIR)/thread_race 4 1000000 mutex
	$(BIN_DIR)/prodcons -P 2 -C 2 -B 64 -N 100000

run_thread: $(BIN_DIR)/thread_race
	$(BIN_DIR)/thread_race 4 1000000 mutex

run_prodcons: $(BIN_DIR)/prodcons
	$(BIN_DIR)/prodcons -P 2 -C 2 -B 64 -N 100000
