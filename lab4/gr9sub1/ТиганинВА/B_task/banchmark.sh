#!/bin/bash

PROG=./B
FILE=testfile.bin
SIZE_MB=100
OUT=output.txt

$PROG $FILE --create-file $SIZE_MB

echo "Benchmark results ($(date))" > $OUT
echo "File: $FILE (${SIZE_MB} MB)" >> $OUT
echo "===================================" >> $OUT

for i in $(seq 1 15); do
    echo -e "\nRun $i:" >> $OUT
    sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
    $PROG $FILE | grep -E "Method|Time elapsed|Minor page faults|Major page faults|Checksum|Verification" >> $OUT
done

echo -e "\nВсе результаты сохранены в $OUT"
