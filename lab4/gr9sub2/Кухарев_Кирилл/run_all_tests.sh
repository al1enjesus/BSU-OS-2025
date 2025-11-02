#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' 

mkdir -p logs

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Лабораторная 4: Запуск всех тестов${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

echo -e "${GREEN}[1/3] Запуск memory_analysis...${NC}"
echo "Задание A: Анализ виртуальной памяти" | tee logs/task_a.log
echo "======================================" | tee -a logs/task_a.log
echo "" | tee -a logs/task_a.log

echo "=== Демонстрационный режим ===" | tee -a logs/task_a.log
./bin/memory_analysis | tee -a logs/task_a.log

echo "" | tee -a logs/task_a.log
echo "=== Анализ процесса bash ===" | tee -a logs/task_a.log
BASH_PID=$(pgrep -f "^/bin/bash" | head -1)
if [ -n "$BASH_PID" ]; then
    echo "PID bash: $BASH_PID" | tee -a logs/task_a.log
    ./bin/memory_analysis $BASH_PID | tee -a logs/task_a.log
else
    echo "Процесс bash не найден" | tee -a logs/task_a.log
fi

echo "" | tee -a logs/task_a.log
echo "=== Системная информация (ps) ===" | tee -a logs/task_a.log
ps -o pid,comm,vsz,rss,pmem | head -10 | tee -a logs/task_a.log

echo -e "${GREEN}✓ Логи сохранены в logs/task_a.log${NC}"
echo ""


echo -e "${GREEN}[2/3] Запуск mmap_benchmark...${NC}"
echo "Задание B: Сравнение mmap() vs read()" | tee logs/task_b.log
echo "======================================" | tee -a logs/task_b.log
echo "" | tee -a logs/task_b.log

if [ ! -f bin/testfile.bin ]; then
    echo "Создание тестового файла..." | tee -a logs/task_b.log
    ./bin/mmap_benchmark bin/testfile.bin --create 100 | tee -a logs/task_b.log
fi

echo "" | tee -a logs/task_b.log
echo "=== Запуск бенчмарка ===" | tee -a logs/task_b.log
./bin/mmap_benchmark bin/testfile.bin | tee -a logs/task_b.log

echo "" | tee -a logs/task_b.log
echo "=== Детальная статистика (/usr/bin/time) ===" | tee -a logs/task_b.log
/usr/bin/time -v ./bin/mmap_benchmark bin/testfile.bin 2>&1 | tee -a logs/task_b_time.log

echo -e "${GREEN}✓ Логи сохранены в logs/task_b.log и logs/task_b_time.log${NC}"
echo ""

echo -e "${GREEN}[3/3] Запуск page_faults...${NC}"
echo "Задание C: Демонстрация page faults" | tee logs/task_c.log
echo "====================================" | tee -a logs/task_c.log
echo "" | tee -a logs/task_c.log

./bin/page_faults | tee -a logs/task_c.log

echo "" | tee -a logs/task_c.log
echo "=== Последовательный доступ (отдельно) ===" | tee -a logs/task_c.log
./bin/page_faults --sequential | tee -a logs/task_c_sequential.log

echo -e "${GREEN}✓ Логи сохранены в logs/task_c.log и logs/task_c_sequential.log${NC}"
echo ""

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Все тесты завершены!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Логи сохранены в директории logs/:"
ls -lh logs/
echo ""
echo -e "${YELLOW}Для скриншотов используйте:${NC}"
echo "  cat logs/task_a.log"
echo "  cat logs/task_b.log"
echo "  cat logs/task_c.log"
