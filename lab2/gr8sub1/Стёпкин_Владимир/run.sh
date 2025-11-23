#!/bin/bash
# Скрипт для сборки и запуска fork_example.c

set -e  # выйти при ошибке

echo "===> Сборка проекта"
make clean
make

echo "===> Запуск программы"
./fork_example
