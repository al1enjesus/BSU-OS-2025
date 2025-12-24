CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGETS = fork ptree

all: $(TARGETS)

fork: fork.c
  $(CC) $(CFLAGS) -o fork fork.c

ptree: ptree.c
  $(CC) $(CFLAGS) -o ptree ptree.c

clean:
  rm -f $(TARGETS)

run_fork: fork
  ./fork

run_ptree: ptree
  ./ptree

.PHONY: all clean run_fork run_ptree
