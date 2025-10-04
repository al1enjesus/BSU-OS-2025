#!/bin/bash

make all

mkdir -p logs
./thread_race 1 10000000 unsync > logs/thread_race_unsync.log
./thread_race 2 10000000 unsync >> logs/thread_race_unsync.log
./thread_race 4 10000000 unsync >> logs/thread_race_unsync.log
./thread_race 8 10000000 unsync >> logs/thread_race_unsync.log

./thread_race 1 10000000 mutex > logs/thread_race_mutex.log
./thread_race 2 10000000 mutex >> logs/thread_race_mutex.log
./thread_race 4 10000000 mutex >> logs/thread_race_mutex.log
./thread_race 8 10000000 mutex >> logs/thread_race_mutex.log

./thread_race 1 10000000 atomic > logs/thread_race_atomic.log
./thread_race 2 10000000 atomic >> logs/thread_race_atomic.log
./thread_race 4 10000000 atomic >> logs/thread_race_atomic.log
./thread_race 8 10000000 atomic >> logs/thread_race_atomic.log

./prodcons -P 4 -C 4 -N 100000 -B 16 > logs/prodcons.log

./thread_race 4 1000000 mutex &
PID=$!
ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm > logs/thread_race_mutex_threads.log
cat /proc/$PID/status | grep Threads >> logs/thread_race_mutex_threads.log
ls -l /proc/$PID/task >> logs/thread_race_mutex_threads.log
kill $PID

./prodcons -P 2 -C 2 -N 100000 -B 64 &
PID=$!
ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm > logs/prodcons_threads.log
cat /proc/$PID/status | grep Threads >> logs/prodcons_threads.log
ls -l /proc/$PID/task >> logs/prodcons_threads.log
kill $PID
