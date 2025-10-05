#!/bin/bash

echo "=== Thread Investigation Script ===" > thread_investigation.txt
echo "Started at: $(date)" >> thread_investigation.txt
echo "" >> thread_investigation.txt

# Запускаем prodcons_sem в фоне с большим N чтобы работал дольше
echo "Starting prodcons_sem in background..." >> thread_investigation.txt
./prodcons_sem -P 3 -C 2 -N 5000000 -B 100 &
PID=$!

echo "Program PID: $PID" >> thread_investigation.txt
echo "" >> thread_investigation.txt

# Ждём немного чтобы программа успела создать потоки
sleep 2

echo "=== 1. Process info ===" >> thread_investigation.txt
ps -p $PID -o pid,ppid,state,comm >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 2. Threads info ===" >> thread_investigation.txt
ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 3. Thread count from /proc ===" >> thread_investigation.txt
cat /proc/$PID/status | grep -E "(Threads|Pid)" >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 4. Task directory ===" >> thread_investigation.txt
ls -l /proc/$PID/task >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 5. CPU info ===" >> thread_investigation.txt
echo "CPU cores: $(nproc)" >> thread_investigation.txt
echo "" >> thread_investigation.txt

# Ждём завершения программы
wait $PID

echo "=== 6. Program finished ===" >> thread_investigation.txt
echo "Ended at: $(date)" >> thread_investigation.txt

echo "Investigation complete! Results saved to thread_investigation.txt"
