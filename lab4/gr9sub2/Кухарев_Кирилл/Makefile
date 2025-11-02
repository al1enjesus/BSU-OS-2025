CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lm

# Директории
SRC = src/
BIN = bin/

# Цели
PROGRAMS = $(BIN)memory_analysis $(BIN)mmap_benchmark $(BIN)page_faults
TEST_FILE = $(BIN)testfile.bin

.PHONY: all clean test help dirs


dirs:
	@mkdir -p $(BIN)

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
	@$(BIN)memory_analysis $$(pgrep -f "^/bin/bash" | head -1 2>/dev/null || echo 1)

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
	@echo "Текущий процесс bash:"
	@PID=$$(pgrep -f "^/bin/bash" | head -1); \
	if [ -n "$$PID" ]; then \
		echo "PID: $$PID"; \
		$(BIN)memory_analysis $$PID; \
		echo ""; \
		echo "Сравнение с ps:"; \
		ps -o pid,comm,vsz,rss -p $$PID; \
		echo ""; \
		echo "Полный статус proc:"; \
		cat /proc/$$PID/status | grep -E "^Vm"; \
	fi

benchmark: $(BIN)mmap_benchmark $(TEST_FILE)
	@echo "=== Бенчмарк производительности с замером времени ==="
	@echo ""
	@echo "Метод 1: Традиционный read()"
	/usr/bin/time -v $(BIN)mmap_benchmark $(TEST_FILE) 2>&1 | grep -E "(elapsed|fault)"
	@echo ""
	@echo "Для очистки кэша страниц (требуется sudo):"
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo "Затем запустите: $(BIN)mmap_benchmark $(TEST_FILE)"

clean:
	rm -rf $(BIN)
	@echo "✓ Удалена директория $(BIN) со всеми файлами"

help:
	@echo "Лаб 4 - Анализ виртуальной памяти и I/O"
	@echo ""
	@echo "Основные цели:"
	@echo "  make all         - Собрать все программы в $(BIN)"
	@echo "  make test-a      - Запустить Задание A (Анализ памяти)"
	@echo "  make test-b      - Запустить Задание B (mmap против read)"
	@echo "  make test-c      - Запустить Задание C (Ошибки страниц)"
	@echo "  make test        - Запустить все демонстрации"
	@echo "  make analyze     - Детальный анализ системной памяти"
	@echo "  make benchmark   - Запустить бенчмарк производительности"
	@echo "  make clean       - Удалить все собранные файлы из $(BIN)"
	@echo "  make help        - Показать эту справку"
	@echo ""
	@echo "Программы в $(BIN):"
	@echo "  $(BIN)memory_analysis [PID]          - Анализировать использование памяти"
	@echo "  $(BIN)mmap_benchmark <файл> [опции]  - Бенчмарк методов I/O"
	@echo "  $(BIN)page_faults [--sequential|--random|--calloc]"
	@echo ""
	@echo "Команды системного анализа:"
	@echo "  ps -o pid,comm,vsz,rss -p <PID>  - Показать статистику памяти"
	@echo "  cat /proc/<PID>/status | grep Vm  - Показать метрики VM"
	@echo "  cat /proc/<PID>/maps              - Показать карту памяти"
	@echo "  /usr/bin/time -v ./program        - Замер времени с ошибками страниц"
	@echo ""
	@echo "Примеры использования:"
	@echo "  make all                  # Собрать всё"
	@echo "  make test                 # Запустить все тесты"
	@echo "  make analyze              # Анализ процесса"
	@echo "  make clean                # Очистить всё"

.PHONY: all clean test help dirs test-a test-b test-c analyze benchmark
