#!/bin/bash

echo "Компиляция программ..."
make clean
make
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции. Тесты прерваны."
    exit 1
fi
echo "Компиляция прошла успешно!"
echo ""


echo "Тестирование thread_race..."

TOTAL_ITERATIONS=8000000

printf "%-10s %-8s %-18s %-12s %-12s %-10s\n" "Mode" "Threads" "Iters per Thread" "Expected" "Actual" "Time (ms)"
echo "--------------------------------------------------------------------------------"

run_race_test() {
    local mode=$1
    local threads=$2
    local iters_per_thread=$((TOTAL_ITERATIONS / threads))

    output=$(./thread_race $threads $iters_per_thread $mode)

    actual=$(echo "$output" | grep -o 'actual=[0-9]*' | cut -d'=' -f2)
    expected=$(echo "$output" | grep -o 'expected=[0-9]*' | cut -d'=' -f2)
    time_ms=$(echo "$output" | grep -o 'time_ms=[0-9]*' | cut -d'=' -f2)

    printf "%-10s %-8s %-18s %-12s %-12s %-10s\n" "$mode" "$threads" "$iters_per_thread" "$expected" "$actual" "$time_ms"
}

for n in 1 2 4 8; do
    run_race_test "unsync" $n
    run_race_test "mutex" $n
    run_race_test "atomic" $n
done
echo ""


echo "Шаг 3: Тестирование prodcons..."
./prodcons -P 4 -C 4 -N 1000000 -B 128
echo ""

echo "Finita les tests!"
