all: process chain

process: src/process.c
		gcc -Wall -o process src/process.c

chain: src/chain.c
		gcc -Wall -o chain src/chain.c

clean:
		rm -f process chain

.PHONY: all clean