CC=gcc
CFLAGS=-Wall -Wextra -O2

SRC=src/fork_example.c
BIN=fork_example

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

