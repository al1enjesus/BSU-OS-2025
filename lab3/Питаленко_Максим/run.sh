#!/bin/bash

echo "=== Вариант 2 — Потоки и синхронизация ==="

echo -e "\n1. Гонка данных (unsync) - 4 потока по 10000 итераций:"
python3 src/thread_race.py 4 10000 unsync

echo -e "\n2. Гонка данных (mutex) - 4 потока по 10000 итераций:"
python3 src/thread_race.py 4 10000 mutex

echo -e "\n3. Гонка данных (atomic) - 4 потока по 10000 итераций:"
python3 src/thread_race.py 4 10000 atomic

echo -e "\n4. Producer-Consumer (маленький тест):"
python3 src/prodcons.py -P 2 -C 2 -N 100 -B 5

echo -e "\n5. Информация о потоках:"
python3 src/threads_info.py
