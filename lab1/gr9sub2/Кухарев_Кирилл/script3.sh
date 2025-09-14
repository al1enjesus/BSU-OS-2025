#!/bin/bash
# Скрипт для вывода топ-5 самых часто устанавливаемых пакетов в Arch Linux

LOG_FILE="/var/log/pacman.log"
OUTPUT="top5_packages.txt"
grep "installed" "$LOG_FILE" > installs.txt

awk '{print $4}' installs.txt | sort | uniq -c | sort -nr | head -n 5 > "$OUTPUT"

# Вывод результата
cat "$OUTPUT"

# Очистка
rm installs.txt
