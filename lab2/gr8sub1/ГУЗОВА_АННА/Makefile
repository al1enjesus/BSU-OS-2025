all: fork_example

fork_example:
	gcc src/fork_example.c -Wall -Wextra -O2 -o fork_example
	
ptree:
	gcc src/ptree.c -Wall -Wextra -O2 -o ptree
  
run_fork_example:
	./fork_example
	
run_ptree:
	./ptree
	
clean:
	rm -f fork_example ptree
