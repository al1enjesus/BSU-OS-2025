#!/bin/bash

# Путь к рабочей директории
WORKDIR="/home/vboxuser/BSU-OS-2025/lab4"

# Очистка старых файлов
rm -f $WORKDIR/metrics.txt $WORKDIR/smaps_rollup_*.txt

# Запуск программы memory_info в фоновом режиме
./memory_info > $WORKDIR/memory_output_debug.txt 2>&1 &
# Получение PID
PID=$(pidof memory_info)
if [ -z "$PID" ]; then
    echo "Ошибка: не удалось запустить memory_info или получить PID"
    exit 1
fi
echo "Запущен memory_info с PID: $PID"

# Функция для сбора метрик
collect_metrics() {
    local STAGE=$1
    echo "Сбор метрик для стадии: $STAGE" >> $WORKDIR/metrics.txt
    echo "----------------------------------------" >> $WORKDIR/metrics.txt
    ps -o pid,comm,vsz,rss -p $PID >> $WORKDIR/metrics.txt
    cat /proc/$PID/status | grep -E "^Vm" >> $WORKDIR/metrics.txt
    cat /proc/$PID/stat | awk '{print "Minor Faults: "$10"\nMajor Faults: "$11}' >> $WORKDIR/metrics.txt
    cat /proc/$PID/smaps_rollup > $WORKDIR/smaps_rollup_$STAGE.txt
    echo "Метрики для $STAGE собраны" >> $WORKDIR/metrics.txt
    echo "" >> $WORKDIR/metrics.txt
}

# Сбор метрик для каждой стадии
collect_metrics "before"
echo "Ожидание 10 секунд для стадии 'After Allocation'..."
sleep 12  # Даём чуть больше времени для надёжности
collect_metrics "after"
echo "Ожидание 10 секунд для стадии 'After Freeing'..."
sleep 12
collect_metrics "freed"

# Ожидание завершения программы
wait $PID
echo "Программа memory_info завершена"
echo "Результаты сохранены в $WORKDIR/metrics.txt и $WORKDIR/smaps_rollup_*.txt"
