#!/bin/bash

echo "=== Лабораторная работа 2 ==="
echo "Сборка и запуск программ..."

make all

echo ""
echo "=== Запуск fork_example ==="
./fork_example

echo ""
echo "=== Запуск ptree для текущего процесса ==="
./ptree $$

echo ""
echo "=== Проверка дерева процессов ==="
echo "pstree -p $$:"
pstree -p $$

echo ""
echo "=== Анализ нагрузки ==="
echo "TOP-5 по CPU:"
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 6

echo ""
echo "TOP-5 по памяти:"
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 6

echo ""
echo "=== Завершение ==="
make clean
