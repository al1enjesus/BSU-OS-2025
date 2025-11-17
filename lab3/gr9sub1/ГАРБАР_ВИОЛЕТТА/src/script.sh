#!/bin/bash

cleanup() {
    echo "=== Cleaning up background processes ===" >> thread_investigation.txt
    if [ ! -z "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        echo "Killing background process $PID" >> thread_investigation.txt
        kill "$PID" 2>/dev/null
        sleep 1
        if kill -0 "$PID" 2>/dev/null; then
            echo "Force killing process $PID" >> thread_investigation.txt
            kill -9 "$PID" 2>/dev/null
        fi
    fi
    echo "Cleanup completed at: $(date)" >> thread_investigation.txt
}

trap cleanup EXIT INT TERM

echo "=== Thread Investigation Script ===" > thread_investigation.txt
echo "Started at: $(date)" >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "Starting prodcons_sem in background..." >> thread_investigation.txt
./prodcons_sem -P 3 -C 2 -N 5000000 -B 100 &
PID=$!

if ! kill -0 "$PID" 2>/dev/null; then
    echo "ERROR: Failed to start prodcons_sem" >> thread_investigation.txt
    exit 1
fi

echo "Program PID: $PID" >> thread_investigation.txt
echo "" >> thread_investigation.txt

sleep 2

if ! kill -0 "$PID" 2>/dev/null; then
    echo "ERROR: Process $PID died prematurely" >> thread_investigation.txt
    exit 1
fi

echo "=== 1. Process info ===" >> thread_investigation.txt
ps -p $PID -o pid,ppid,state,comm 2>/dev/null >> thread_investigation.txt || echo "Process not found" >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 2. Threads info ===" >> thread_investigation.txt
ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm 2>/dev/null >> thread_investigation.txt || echo "Cannot get thread info" >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "=== 3. Thread count from /proc ===" >> thread_investigation.txt
if [ -d "/proc/$PID" ]; then
    cat /proc/$PID/status | grep -E "(Threads|Pid)" >> thread_investigation.txt
else
    echo "Process directory /proc/$PID not found" >> thread_investigation.txt
fi
echo "" >> thread_investigation.txt

echo "=== 4. Task directory ===" >> thread_investigation.txt
if [ -d "/proc/$PID/task" ]; then
    ls -l /proc/$PID/task >> thread_investigation.txt
else
    echo "Task directory /proc/$PID/task not found" >> thread_investigation.txt
fi
echo "" >> thread_investigation.txt

echo "=== 5. CPU info ===" >> thread_investigation.txt
echo "CPU cores: $(nproc)" >> thread_investigation.txt
echo "" >> thread_investigation.txt

echo "Waiting for program completion..." >> thread_investigation.txt
for i in {1..30}; do
    if ! kill -0 "$PID" 2>/dev/null; then
        break
    fi
    sleep 1
done

if kill -0 "$PID" 2>/dev/null; then
    echo "Program taking too long, terminating..." >> thread_investigation.txt
    cleanup
fi

echo "=== 6. Program finished ===" >> thread_investigation.txt
echo "Ended at: $(date)" >> thread_investigation.txt

echo "Investigation complete! Results saved to thread_investigation.txt"
