#!/bin/bash

OUTPUT_FILE="prodcons_result.txt"
PROGRAM=./prodcons

make clean
make build_prodcons

> "$OUTPUT_FILE"

TESTS=(
  "-P 1 -C 1 -N 1000   -B 16"
  "-P 2 -C 2 -N 10000  -B 32"
  "-P 4 -C 2 -N 50000  -B 64"
  "-P 2 -C 4 -N 50000  -B 128"
  "-P 3 -C 3 -N 75000  -B 10"
  "-P 5 -C 2 -N 20000  -B 50"
  "-P 2 -C 5 -N 20000  -B 20"
  "-P 8 -C 8 -N 80000  -B 256"
  "-P 1 -C 4 -N 4000   -B 8"
  "-P 6 -C 3 -N 60000  -B 100"
)

i=1
for args in "${TESTS[@]}"; do
  echo "=== Test $i: $args ===" | tee -a "$OUTPUT_FILE"
  $PROGRAM $args | tee -a "$OUTPUT_FILE"
  echo "" >> "$OUTPUT_FILE"
  ((i++))
done

make clean

echo "Все тесты завершены. Результаты сохранены в $OUTPUT_FILE"
