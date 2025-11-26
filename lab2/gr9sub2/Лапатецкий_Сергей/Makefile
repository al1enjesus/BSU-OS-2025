.PHONY: clean 

all: fork_example ptree
	 ./ptree && ./fork_example

fork_example.o: ./src/fork_example.c
	g++ -Wall -Wextra -O2 -c -o fork_example.o ./src/fork_example.c

ptree.o: ./src/ptree.c
	g++ -Wall -Wextra -O2 -c -o ptree.o ./src/ptree.c

fork_example: fork_example.o
	g++ fork_example.o -o fork_example

ptree: ptree.o
	g++ ptree.o -o ptree

run_fork_example: fork_example
	./fork_example

run_ptree: ptree
	./ptree

clean:
	rm -rf fork_example ptree *.o
