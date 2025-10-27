#!/bin/bash

sizes=(50 100 500)  # массив только нужных размеров

for i in {0..2}; do  # индексы 0,1,2
    for size in "${sizes[@]}"; do
        echo "=== Running with size: $size MB. Take: $i ==="
        ./bin/io_benchmark --size $size > ./logs/io_benchmark/${size}MB_$((i+1)).log
    done
done