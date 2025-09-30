CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread

N ?= 8
M ?= 100000
MODE ?= unsync

.PHONY: all build_race clean run_race build_prodcons run_prodcons

#all: build

build_race: race.c
	gcc $(CFLAGS) race.c -o race

run_race: race
	./race $(N) $(M) $(MODE)

build_prodcons: prodcons.c
	gcc $(CFLAGS) prodcons.c -o prodcons

run_prodcons: prodcons
	./prodcons -P 2 -C 2 -N 100000 -B 64

clean:
	rm -f race
	rm -f prodcons
