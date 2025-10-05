#!/bin/bash

echo "=== Data Race Benchmark Results - $(date) ==="
echo ""

THREADS=(1 2 4 8)
ITERATIONS=2000000
RUNS=3

echo "Running benchmarks: M=$ITERATIONS, $RUNS runs per config"
echo ""

# Запускаем тесты
for mode in unsync mutex atomic; do
    echo "=== Mode: $mode ==="
    for n in "${THREADS[@]}"; do
        for ((i=1; i<=$RUNS; i++)); do
            python3 thread_race.py $n $ITERATIONS $mode
        done
        echo ""
    done
done

echo "Benchmark completed!"
