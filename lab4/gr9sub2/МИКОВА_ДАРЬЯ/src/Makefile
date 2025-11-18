# Makefile для Lab 4 - OS
# Структура:
#   .
#   ├── Makefile
#   └── src/
#       ├── memory_profiler_working.c
#       ├── analyze_memory.c
#       ├── memory_benchmark.c
#       └── page_faults_fixed.c

CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC_DIR = src
BIN_DIR = bin

# Программы
PROFILER = $(BIN_DIR)/memory_profiler
ANALYZE = $(BIN_DIR)/analyze_memory
BENCHMARK = $(BIN_DIR)/memory_benchmark
PAGE_FAULTS = $(BIN_DIR)/page_faults

# Исходные файлы
PROFILER_SRC = $(SRC_DIR)/memory_profiler.c
ANALYZE_SRC = $(SRC_DIR)/analyze_memory.c
BENCHMARK_SRC = $(SRC_DIR)/memory_benchmark.c
PAGE_FAULTS_SRC = $(SRC_DIR)/page_faults.c

# Параметры для запуска
PID ?= $$$$
WATCH_TIME ?= 10
COMPARE_PID1 ?= 1
COMPARE_PID2 ?= 2

.PHONY: all clean run_profiler run_analyze_memory run_benchmark run_page_faults help

# ============================================
# Основные цели
# ============================================

all: $(BIN_DIR) $(PROFILER) $(ANALYZE) $(BENCHMARK) $(PAGE_FAULTS)
	@echo "✓ Все программы скомпилированы"
	@echo ""
	@echo "Скомпилированные программы:"
	@echo "  $(PROFILER)"
	@echo "  $(ANALYZE)"
	@echo "  $(BENCHMARK)"
	@echo "  $(PAGE_FAULTS)"

# Создание директории для бинарников
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# ============================================
# Компиляция программ
# ============================================

$(PROFILER): $(PROFILER_SRC)
	@echo "Компилирую memory_profiler..."
	@$(CC) $(CFLAGS) $< -o $@
	@echo "✓ $(PROFILER) готов"

$(ANALYZE): $(ANALYZE_SRC)
	@echo "Компилирую analyze_memory..."
	@$(CC) $(CFLAGS) $< -o $@
	@echo "✓ $(ANALYZE) готов"

$(BENCHMARK): $(BENCHMARK_SRC)
	@echo "Компилирую memory_benchmark..."
	@$(CC) $(CFLAGS) $< -o $@
	@echo "✓ $(BENCHMARK) готов"

$(PAGE_FAULTS): $(PAGE_FAULTS_SRC)
	@echo "Компилирую page_faults..."
	@$(CC) $(CFLAGS) $< -o $@
	@echo "✓ $(PAGE_FAULTS) готов"

# ============================================
# Цели запуска программ
# ============================================

run_profiler: $(PROFILER)
	@echo "Запуск memory_profiler (PID: $(PID))"
	@echo ""
	@echo "Доступные команды:"
	@echo "  make run_profiler PID=1234              # показать метрики"
	@echo "  make run_profiler PID=1234 WATCH_TIME=30 # мониторить 30 сек"
	@echo ""
	$(PROFILER) $(PID)

run_profiler_watch: $(PROFILER)
	@echo "Запуск memory_profiler в режиме мониторинга..."
	@echo "PID: $(PID), время: $(WATCH_TIME) сек"
	@echo ""
	$(PROFILER) --watch $(PID) $(WATCH_TIME)

run_profiler_compare: $(PROFILER)
	@echo "Сравнение двух процессов..."
	@echo "PID1: $(COMPARE_PID1), PID2: $(COMPARE_PID2)"
	@echo ""
	$(PROFILER) --compare $(COMPARE_PID1) $(COMPARE_PID2)

run_analyze_memory: $(ANALYZE)
	@echo "Запуск analyze_memory (PID: $(PID))"
	@echo ""
	@echo "Доступные команды:"
	@echo "  make run_analyze_memory                 # анализировать текущий процесс"
	@echo "  make run_analyze_memory PID=1234        # анализировать процесс 1234"
	@echo ""
	$(ANALYZE)

run_benchmark: $(BENCHMARK)
	@echo "Запуск memory_benchmark (сравнение read vs mmap)"
	@echo ""
	@echo "Убедитесь, что создан testfile.bin:"
	@echo "  dd if=/dev/urandom of=testfile.bin bs=1M count=100"
	@echo ""
	@echo "Очистка page cache перед запуском..."
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo ""
	@echo "Запуск программы..."
	@echo ""
	$(BENCHMARK)

run_page_faults: $(PAGE_FAULTS)
	@echo "Запуск page_faults_demo (демонстрация page faults)"
	@echo ""
	@echo "Программа покажет:"
	@echo "  - Minor page faults при последовательном доступе"
	@echo "  - Major page faults при случайном доступе"
	@echo ""
	@echo "Для чистых результатов очистите кэш перед запуском:"
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo ""
	@echo "Запуск программы..."
	@echo ""
	$(PAGE_FAULTS)

# ============================================
# Утилиты
# ============================================

clean:
	@echo "Удаление скомпилированных программ..."
	@rm -rf $(BIN_DIR)
	@echo "✓ Очистка завершена"

help:
	@echo "=== Lab 4 - Виртуальная память и I/O ==="
	@echo ""
	@echo "Основные цели:"
	@echo "  all                          - скомпилировать все программы"
	@echo "  clean                        - удалить скомпилированные файлы"
	@echo "  help                         - показать эту справку"
	@echo ""
	@echo "Цели для запуска:"
	@echo ""
	@echo "  run_profiler                 - запустить memory_profiler"
	@echo "    Параметры:"
	@echo "      PID=<pid>                - анализируемый процесс (по умолчанию $$)"
	@echo "      make run_profiler_watch PID=1234 WATCH_TIME=30"
	@echo "      make run_profiler_compare COMPARE_PID1=1 COMPARE_PID2=2"
	@echo ""
	@echo "  run_analyze_memory           - запустить analyze_memory"
	@echo "    Параметры:"
	@echo "      PID=<pid>                - анализируемый процесс (по умолчанию текущий)"
	@echo ""
	@echo "  run_benchmark                - запустить memory_benchmark"
	@echo "    Требует: testfile.bin (100MB)"
	@echo "    Сравнивает: read() vs mmap()"
	@echo ""
	@echo "  run_page_faults              - запустить page_faults_demo"
	@echo "    Демонстрирует: minor и major page faults"
	@echo ""
	@echo "Примеры использования:"
	@echo ""
	@echo "  # Компилировать все"
	@echo "  make"
	@echo ""
	@echo "  # Показать метрики текущего процесса bash"
	@echo "  make run_profiler PID=\$\$(pgrep bash)"
	@echo ""
	@echo "  # Мониторить процесс 1234 в течение 30 секунд"
	@echo "  make run_profiler_watch PID=1234 WATCH_TIME=30"
	@echo ""
	@echo "  # Сравнить процессы 1234 и 5678"
	@echo "  make run_profiler_compare COMPARE_PID1=1234 COMPARE_PID2=5678"
	@echo ""
	@echo "  # Запустить benchmark (нужен testfile.bin)"
	@echo "  dd if=/dev/urandom of=testfile.bin bs=1M count=100"
	@echo "  sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	@echo "  make run_benchmark"
	@echo ""
	@echo "  # Очистить скомпилированные файлы"
	@echo "  make clean"

# ============================================
# Специальные команды для подготовки
# ============================================

prepare_benchmark:
	@echo "Подготовка к запуску benchmark..."
	@if [ ! -f testfile.bin ]; then \
		echo "Создание testfile.bin (100MB)..."; \
		dd if=/dev/urandom of=testfile.bin bs=1M count=100; \
	else \
		echo "testfile.bin уже существует"; \
	fi
	@echo "✓ Готово"

drop_cache:
	@echo "Очистка page cache..."
	@sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
	@echo "✓ Page cache очищен"

test_all: all
	@echo ""
	@echo "=== Тестирование всех программ ==="
	@echo ""
	@echo "1. Анализ памяти текущего процесса..."
	$(ANALYZE)
	@echo ""
	@echo "2. Page Faults демонстрация..."
	$(PAGE_FAULTS)
	@echo ""
	@echo "✓ Все тесты завершены"