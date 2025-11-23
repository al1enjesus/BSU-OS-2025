CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGETS = fork_example ptree

all: $(TARGETS)

fork_example: src/fork_example.c
	$(CC) $(CFLAGS) -o fork_example src/fork_example.c

ptree: src/ptree.c
	$(CC) $(CFLAGS) -o ptree src/ptree.c

run_fork: fork_example
	./fork_example

run_ptree: ptree
	./ptree

clean:
	rm -f $(TARGETS)

help:
	@echo "Доступные цели:"
	@echo "  all      - собрать все программы"
	@echo "  run_fork - запустить пример с процессами"
	@echo "  run_ptree - запустить утилиту ptree"
	@echo "  clean    - удалить скомпилированные файлы"

.PHONY: all clean help run_fork run_ptree


