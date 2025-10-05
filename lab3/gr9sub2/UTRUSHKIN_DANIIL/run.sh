#!/bin/bash

echo "=== Building projects ==="
make all

echo -e "\n=== Testing thread_race with race condition ==="
./thread_race 4 1000000 unsync

echo -e "\n=== Testing thread_race with mutex ==="
./thread_race 4 1000000 mutex

echo -e "\n=== Testing thread_race with atomic ==="
./thread_race 4 1000000 atomic

echo -e "\n=== Testing producer-consumer ==="
./prodcons -P 2 -C 2 -N 100000 -B 64

echo -e "\n=== All tests completed ==="
