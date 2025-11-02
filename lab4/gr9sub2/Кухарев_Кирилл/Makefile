CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRC = src/
BIN = bin/

PROGRAMS = $(BIN)memory_analysis $(BIN)mmap_benchmark $(BIN)page_faults
TEST_FILE = $(BIN)testfile.bin

.PHONY: all clean test help dirs test-a test-b test-c analyze benchmark log-all log-a log-b log-c clean-logs

dirs:
	@mkdir -p $(BIN)
	@mkdir -p logs

all: dirs $(PROGRAMS)
	@echo "✓ Все программы собраны в $(BIN)"

$(BIN)memory_analysis: $(SRC)memory_analysis.c dirs
	$(CC) $(CFLAGS) -o $@ $<
	@echo "✓ Собрано $@"

$(BIN)mmap_benchmark: $(SRC)mmap_benchmark.c dirs
	$(CC) $(CFLAGS) -o $@ $<
	@echo "✓ Собрано $@"

$(BIN)page_faults: $(SRC)page_faults.c dirs
	$(CC) $(CFLAGS) -o $@ $<
	@echo "✓ Собрано $@"

$(TEST_FILE): $(BIN)mmap_benchmark
	@if [ ! -f $@ ]; then \
		$(BIN)mmap_benchmark $@ --create 100; \
	else \
		echo "Тестовый файл уже существует: $@"; \
	fi

test-a: $(BIN)memory_analysis
	@echo "=== Запуск Задания A: Анализ памяти ==="
	@echo ""
	@echo "1. Режим демонстрации (анализ этой программы):"
	$(BIN)memory_analysis
	@echo ""
	@echo "2. Анализ оболочки bash:"
	@BASH_PID=$$(pgrep -o bash 2>/dev/null); \
	if [ -n "$$BASH_PID" ] && [ "$$BASH_PID" != "0" ]; then \
		echo "Найден процесс bash с PID: $$BASH_PID"; \
		$(BIN)memory_analysis $$BASH_PID; \
	else \
		echo "ВНИМАНИЕ: Процесс bash не найден, анализируем init (PID 1)"; \
		$(BIN)memory_analysis 1; \
	fi

test-b: $(BIN)mmap_benchmark $(TEST_FILE)
	@echo "=== Запуск Задания B: mmap против read() ==="
	@echo ""
	@echo "Тестирование производительности read() против mmap()..."
	@echo "Примечание: Запускайте с очищенным кэшем страниц для лучших результатов:"
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo ""
	$(BIN)mmap_benchmark $(TEST_FILE)

test-c: $(BIN)page_faults
	@echo "=== Запуск Задания C: Демонстрация ошибок страниц ==="
	@echo ""
	$(BIN)page_faults

test: test-a test-b test-c

analyze: $(BIN)memory_analysis
	@echo "=== Детальный системный анализ ==="
	@echo ""
	@BASH_PID=$$(pgrep -o bash 2>/dev/null); \
	if [ -z "$$BASH_PID" ] || [ "$$BASH_PID" = "0" ]; then \
		echo "ОШИБКА: Процесс bash не найден"; \
		echo "Доступные процессы:"; \
		ps aux | head -10; \
		exit 1; \
	fi; \
	echo "Анализируем процесс bash (PID: $$BASH_PID)"; \
	echo ""; \
	$(BIN)memory_analysis $$BASH_PID; \
	echo ""; \
	echo "Сравнение с ps:"; \
	ps -o pid,comm,vsz,rss -p $$BASH_PID; \
	echo ""; \
	echo "Полный статус proc:"; \
	cat /proc/$$BASH_PID/status | grep -E "^Vm"

benchmark: $(BIN)mmap_benchmark $(TEST_FILE)
	@echo "=== Бенчмарк производительности с замером времени ==="
	@echo ""
	@echo "Метод 1: Традиционный read()"
	/usr/bin/time -v $(BIN)mmap_benchmark $(TEST_FILE) 2>&1 | grep -E "(elapsed|fault)"
	@echo ""
	@echo "Для очистки кэша страниц (требуется sudo):"
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo "Затем запустите: $(BIN)mmap_benchmark $(TEST_FILE)"

log-all: all
	@echo "=== Запуск всех тестов с логированием ==="
	@mkdir -p logs
	@echo "Задание A..." | tee logs/task_a.log
	@$(MAKE) test-a 2>&1 | tee -a logs/task_a.log
	@echo "" | tee -a logs/task_a.log
	@echo "Задание B..." | tee logs/task_b.log
	@$(MAKE) test-b 2>&1 | tee -a logs/task_b.log
	@echo "" | tee -a logs/task_b.log
	@echo "Задание C..." | tee logs/task_c.log
	@$(MAKE) test-c 2>&1 | tee -a logs/task_c.log
	@echo "✓ Все логи сохранены в logs/"

log-a: $(BIN)memory_analysis
	@mkdir -p logs
	@$(MAKE) test-a 2>&1 | tee logs/task_a_$(shell date +%Y%m%d_%H%M%S).log

log-b: $(BIN)mmap_benchmark $(TEST_FILE)
	@mkdir -p logs
	@$(MAKE) test-b 2>&1 | tee logs/task_b_$(shell date +%Y%m%d_%H%M%S).log

log-c: $(BIN)page_faults
	@mkdir -p logs
	@$(MAKE) test-c 2>&1 | tee logs/task_c_$(shell date +%Y%m%d_%H%M%S).log

clean-logs:
	rm -rf logs/*.log
	@echo "✓ Логи очищены"

clean:
	rm -rf $(BIN)
	@echo "✓ Удалена директория $(BIN) со всеми файлами"

help:
	@echo "Лаб 4 - Анализ виртуальной памяти и I/O"
	@echo ""
	@echo "Основные цели:"
	@echo "  make all         - Собрать все программы в $(BIN)"
	@echo "  make test        - Запустить все демонстрации"
	@echo "  make test-a      - Запустить Задание A (Анализ памяти)"
	@echo "  make test-b      - Запустить Задание B (mmap против read)"
	@echo "  make test-c      - Запустить Задание C (Ошибки страниц)"
	@echo ""
	@echo "Логирование:"
	@echo "  make log-all     - Запустить все с логированием"
	@echo "  make log-a       - Логировать задание A"
	@echo "  make log-b       - Логировать задание B"
	@echo "  make log-c       - Логировать задание C"
	@echo "  make clean-logs  - Удалить все логи"
	@echo ""
	@echo "Анализ:"
	@echo "  make analyze     - Детальный анализ системной памяти"
	@echo "  make benchmark   - Запустить бенчмарк производительности"
	@echo ""
	@echo "Очистка:"
	@echo "  make clean       - Удалить все собранные файлы из $(BIN)"
	@echo "  make help        - Показать эту справку"
	@echo ""
	@echo "Программы в $(BIN):"
	@echo "  $(BIN)memory_analysis [PID]          - Анализировать использование памяти"
	@echo "  $(BIN)mmap_benchmark <файл> [опции]  - Бенчмарк методов I/O"
	@echo "  $(BIN)page_faults [--sequential|--random|--calloc]"
