#!/bin/bash

make clean
make build

RESULT_FILE="threads_result.txt"
echo "Benchmark results for race program" > "$RESULT_FILE"
echo "===================================" >> "$RESULT_FILE"
echo "" >> "$RESULT_FILE"

MODES=("unsync" "mutex" "atomic")
THREADS=(2 4 8 16 32 64)
ITERATIONS=(10 100 1000 10000 100000 1000000 10000000)

for mode in "${MODES[@]}"; do
    for n in "${THREADS[@]}"; do
        for m in "${ITERATIONS[@]}"; do
            echo "Running: mode=$mode, threads=$n, iterations=$m"
            output=$(make run N=$n M=$m MODE=$mode | tee /dev/tty)
            echo "Mode: $mode | Threads: $n | Iterations: $m" >> "$RESULT_FILE"
            echo "$output" >> "$RESULT_FILE"
            echo "" >> "$RESULT_FILE"
        done
    done
done

echo "✅ Benchmark complete. Results saved to $RESULT_FILE"
