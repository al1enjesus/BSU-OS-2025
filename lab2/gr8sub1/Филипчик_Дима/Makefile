CC=gcc
CFLAGS=-Wall -Wextra -std=c99

all: fork_example ptree

fork_example: src/fork_example.c
	$(CC) $(CFLAGS) -o fork_example src/fork_example.c

ptree: src/ptree.c
	$(CC) $(CFLAGS) -o ptree src/ptree.c

clean:
	rm -f fork_example ptree

run: fork_example
	./fork_example

run_ptree: ptree
	./ptree

.PHONY: all clean run run_ptree
