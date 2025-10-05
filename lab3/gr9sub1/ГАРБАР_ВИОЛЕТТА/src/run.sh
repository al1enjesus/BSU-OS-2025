#!/bin/bash

echo "=== Building project ==="
make clean
make all

echo -e "\n=== Testing thread_race (data race demonstration) ==="
echo "Unsynchronized mode (expect race condition):"
./thread_race 4 1000000 unsync

echo -e "\nMutex mode (correct synchronization):"
./thread_race 4 1000000 mutex

echo -e "\nAtomic mode (lock-free synchronization):"
./thread_race 4 1000000 atomic

echo -e "\n=== Testing producer-consumer with semaphores ==="
./prodcons_sem -P 2 -C 2 -N 100000 -B 64

echo -e "\n=== Process/thread inspection commands ==="
echo "To inspect threads in the system, run these commands in another terminal:"
echo "1. Find PID: ps aux | grep prodcons_sem"
echo "2. Show threads: ps -L -p <PID> -o pid,tid,psr,pcpu,stat,comm"
echo "3. Thread count: cat /proc/<PID>/status | grep Threads"
echo "4. Task list: ls -l /proc/<PID>/task"
