#!/bin/bash

echo "=== Data Race Benchmark - $(date) ==="
echo ""


THREADS=(1 2 4 8)
ITERATIONS=2000000
RUNS=3

if [[ ! -f "thread_race.py" ]]; then
    echo "❌ Ошибка: файл thread_race.py не найден!"
    echo "Запускайте скрипт из папки src/ или укажите правильный путь"
    exit 1
fi

echo "Configuration:"
echo "  Thread counts: ${THREADS[*]}"
echo "  Total increments: $ITERATIONS"
echo "  Runs per configuration: $RUNS"
echo ""

run_tests() {
    local mode=$1
    
    echo "=== Testing $mode mode ==="
    
    for n in "${THREADS[@]}"; do
        echo "Threads: $n"
        
        for ((run=1; run<=RUNS; run++)); do
            if [[ -f "thread_race.py" ]]; then
                python3 thread_race.py $n $ITERATIONS $mode
            else
                echo "❌ Файл thread_race.py удален во время выполнения!"
                exit 1
            fi
        done
        echo ""
    done
}

for mode in unsync mutex atomic; do
    run_tests "$mode"
done

echo "Benchmark completed!"
