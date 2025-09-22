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

run: all
	./fork_example
	./ptree $$

test: all
	./run.sh

.PHONY: all clean run test
