#!/bin/bash

echo "Компиляция..."
gcc memory_info.c -o memory_info

echo "Запуск программы..."
./memory_info &
PID=$!
echo "PID: $PID"
sleep 12  # Ждём "Before Allocation"

echo -e "\n=== ДО ВЫДЕЛЕНИЯ ==="
ps -o pid,comm,vsz,rss -p $PID
echo
cat /proc/$PID/status | grep -E "^Vm"
echo
cat /proc/$PID/smaps_rollup | grep -E "Pss|Private_Dirty"

echo -e "\nНажми Enter в терминале с программой, чтобы выделить память..."
read -p "Готово? Нажми Enter здесь тоже..." 

sleep 12  # Ждём "After Allocation"

echo -e "\n=== ПОСЛЕ ВЫДЕЛЕНИЯ (ПРОЦЕСС ЕЩЁ ЖИВ!) ==="
ps -o pid,comm,vsz,rss -p $PID
echo
cat /proc/$PID/status | grep -E "^Vm"
echo
cat /proc/$PID/smaps_rollup | grep -E "Pss|Private_Dirty"

echo -e "\nТеперь нажми Enter в терминале с программой, чтобы освободить и выйти..."
read -p "Готово? Нажми Enter здесь..." 

# Убиваем процесс (на всякий случай)
kill $PID 2>/dev/null
echo -e "\nГотово! Проверка завершена."
