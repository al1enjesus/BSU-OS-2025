#!/bin/bash
echo "=== ЗАДАНИЕ A: Анализ виртуальной памяти ==="

# Запускаем и сразу анализируем
echo "Запускаем memory_info..."
./memory_info &
PID=$!
echo "PID процесса: $PID"

# Немедленный анализ - не ждем
echo -e "\n1. Анализ через ps:"
ps -o pid,vsz,rss,comm -p $PID 2>/dev/null || echo "Процесс уже завершился"

echo -e "\n2. Попытка анализа через /proc:"
if [ -d "/proc/$PID" ]; then
    echo "=== Детальная информация из /proc/$PID/status ==="
    cat /proc/$PID/status | grep -E "^Vm"
    
    echo -e "\n=== Карта памяти (/proc/$PID/maps) ==="
    cat /proc/$PID/maps | head -15
else
    echo "Процесс $PID уже завершился"
    echo "Это нормально - программа быстро отрабатывает"
fi

echo -e "\n3. Анализ завершен"
