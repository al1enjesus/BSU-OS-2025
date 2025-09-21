#!/bin/bash

read -p "Введите PID процесса: " pid
if [ ! -d "/proc/$pid" ]; then
    echo "Ошибка: Процесс с PID $pid не существует."
    exit 1
fi
echo "Статус процесса:"
cat /proc/$pid/cmdline | tr '\0' ' '; echo
head -n 20 /proc/$pid/status
ls -l /proc/$pid/fd

