CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = fork_example ptree

all: $(TARGETS)

fork_example: fork_example.c
	$(CC) $(CFLAGS) -o fork_example fork_example.c

ptree: ptree.c
	$(CC) $(CFLAGS) -o ptree ptree.c

clean:
	rm -f $(TARGETS)

run_fork: fork_example
	./fork_example

run_ptree: ptree
	./ptree

.PHONY: all clean run_fork run_ptree
