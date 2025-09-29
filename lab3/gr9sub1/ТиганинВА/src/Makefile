CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread

N ?= 8
M ?= 100000
MODE ?= unsync

.PHONY: all build clean

all: build

build: race.c
	gcc $(CFLAGS) race.c -o race

run: race
	./race $(N) $(M) $(MODE)

clean:
	rm -f race
