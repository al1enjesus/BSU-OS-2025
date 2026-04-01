all: fork_example ptree

fork_example: src/fork_example.c
	gcc -Wall -o fork_example src/fork_example.c

ptree: src/ptree.c
	gcc -Wall -o ptree src/ptree.c

run: fork_example ptree
	./fork_example
	./ptree

clean:
	rm -f fork_example ptree

.PHONY: all run clean