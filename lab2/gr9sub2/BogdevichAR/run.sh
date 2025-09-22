#!/bin/bash

echo "=== Лабораторная работа 2 ==="
echo "Сборка и запуск программ..."
echo ""

# Сборка программ
make all

echo "=== Запуск fork_example ==="
./fork_example

echo ""
echo "=== Запуск ptree для текущего процесса ==="
./ptree $$

echo ""
echo "=== Тест ptree с разными PID ==="
./ptree 1
./ptree $$

echo ""
echo "=== Завершение ==="
make clean

echo ""
echo "=== Проверка дерева процессов ==="
echo "Текущее дерево процессов:"
ps -ef --forest | head -n 15
