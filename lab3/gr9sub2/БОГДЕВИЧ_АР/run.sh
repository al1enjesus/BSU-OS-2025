#!/bin/bash

echo "=== Лабораторная работа 3 - Потоки, синхронизация и гонки данных ==="
echo "Вариант 1 - Богдевич А.Р."

echo -e "\n--- Часть A: Гонка данных ---"
echo "Без синхронизации:"
./thread_race 4 1000000 unsync

echo -e "\nС мьютексом:"
./thread_race 4 1000000 mutex

echo -e "\nС атомиками:"
./thread_race 4 1000000 atomic

echo -e "\n--- Часть B: Producer-Consumer ---"
./prodcons -P 2 -C 2 -N 100000 -B 64

echo -e "\n--- Часть C: Потоки в системе ---"
echo "Запустите следующие команды в другом терминале во время работы программы:"
echo "ps -L -p <PID> -o pid,tid,psr,pcpu,stat,comm | head -n 20"
echo "cat /proc/<PID>/status | grep Threads"
echo "ls -l /proc/<PID>/task | head -n 10"

