CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = fork_example ptree
SRCDIR = src
BINDIR = .

all: $(TARGETS)

fork_example: $(SRCDIR)/fork_example.c
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $<

ptree: $(SRCDIR)/ptree.c
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $<

clean:
	rm -f $(TARGETS)

run_fork:
	./fork_example

run_ptree:
	./ptree

run_ptree_current:
	./ptree $$

test: all
	@echo "=== Testing fork_example ==="
	./fork_example
	@echo "=== Testing ptree ==="
	./ptree $$

.PHONY: all clean run_fork run_ptree run_ptree_current test
