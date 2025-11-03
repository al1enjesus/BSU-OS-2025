# Компиляция: make all
# Очистка: make clean

# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lrt

# Цели
TARGETS = memory_info io_benchmark memory_profiler

# Основная цель - сборка всех программ
all: $(TARGETS)

# Сборка отдельных программ
memory_info: memory_info.c
	$(CC) $(CFLAGS) -o $@ $<

io_benchmark: io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

# Цели для запуска примеров
run_memory_info: memory_info
	@echo "=== Запуск memory_info ==="
	./memory_info

run_io_benchmark: io_benchmark
	@echo "=== Запуск io_benchmark ==="
	./io_benchmark --size 50

run_profiler: memory_profiler
	@echo "=== Запуск memory_profiler на текущем процессе ==="
	./memory_profiler $$

# Тестирование
test: all
	@echo "=== Запуск всех тестов ==="
	@echo "1. Testing memory_info..."
	@./memory_info --help > /dev/null 2>&1 && echo "memory_info OK" || echo "memory_info FAILED"
	@echo "2. Testing memory_profiler..."
	@./memory_profiler $$ --help > /dev/null 2>&1 && echo "memory_profiler OK" || echo "memory_profiler FAILED"
	@echo "=== Тестирование завершено ==="

# Бенчмарк производительности
benchmark: io_benchmark
	@echo "=== Запуск бенчмарков ==="
	@echo "I/O Benchmark..."
	@./io_benchmark --size 10
	@echo ""

# Создание тестовых файлов
test_files:
	@echo "=== Создание тестовых файлов ==="
	@dd if=/dev/urandom of=testfile_100mb.bin bs=1M count=100 status=none
	@echo "Создан testfile_100mb.bin (100 MB)"
	@dd if=/dev/urandom of=testfile_500mb.bin bs=1M count=500 status=none
	@echo "Создан testfile_500mb.bin (500 MB)"

# Очистка page cache (требует root)
clean_cache:
	@echo "=== Очистка page cache ==="
	@sync
	@sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches' || echo "Требуются права root для очистки cache"

# Мониторинг системы во время тестов
monitor:
	@echo "=== Запуск мониторинга системы ==="
	@echo "Откройте другой терминал для мониторинга:"
	@echo "  htop"
	@echo "  watch -n 1 'free -h'"
	@echo "  iostat -x 1"

# Демонстрация всех возможностей
demo: all
	@echo "=== Демонстрация всех программ ==="
	@echo "1. Memory Info Demo..."
	@./memory_info | head -15
	@echo ""
	@echo "2. Memory Profiler Demo..."
	@./memory_profiler $$ --map | head -30

# Проверка зависимостей
check_deps:
	@echo "=== Проверка зависимостей ==="
	@which $(CC) > /dev/null && echo "gcc установлен" || echo "gcc не установлен"
	@which strace > /dev/null && echo "strace установлен" || echo "strace не установлен"
	@which htop > /dev/null && echo "htop установлен" || echo "htop не установлен"

# Установка зависимостей (для Ubuntu/Debian)
install_deps:
	@echo "=== Установка зависимостей ==="
	sudo apt update
	sudo apt install -y strace htop iotop sysstat

# Отладочная сборка
debug: CFLAGS = -Wall -Wextra -g -DDEBUG -std=c99
debug: clean all

# Статический анализ кода
analyze: clean
	@echo "=== Статический анализ кода ==="
	@for target in $(TARGETS); do \
		echo "Анализ $$target.c..."; \
		$(CC) $(CFLAGS) -fsyntax-only $$target.c && echo "Синтаксис OK" || echo "Ошибки синтаксиса"; \
	done

# Проверка утечек памяти
valgrind_check: debug
	@echo "=== Проверка утечек памяти ==="
	@for target in $(TARGETS); do \
		if [ -f $$target ]; then \
			echo "Проверка $$target..."; \
			valgrind --leak-check=full ./$$target --help > /dev/null 2>&1; \
		fi \
	done

# Профилирование
profile: CFLAGS = -Wall -Wextra -pg -std=c99
profile: clean all
	@echo "=== Сборка для профилирования ==="
	@echo "Запустите программу, затем: gprof <program> gmon.out > analysis.txt"

# Создание архива для сдачи
package: clean
	@echo "=== Создание архива для сдачи ==="
	@tar -czf lab4_submission.tar.gz *.c *.md Makefile

# Очистка
clean:
	@echo "=== Очистка ==="
	rm -f $(TARGETS)
	rm -f *.o
	rm -f test*.bin
	rm -f benchmark_file.bin
	rm -f gmon.out
	rm -f *.tar.gz

# Справка
help:
	@echo "Доступные цели:"
	@echo "  all              - Сборка всех программ"
	@echo "  clean            - Очистка скомпилированных файлов"
	@echo "  test             - Запуск тестов"
	@echo "  benchmark        - Запуск бенчмарков производительности"
	@echo "  demo             - Демонстрация всех программ"
	@echo "  check_deps       - Проверка зависимостей"
	@echo "  install_deps     - Установка зависимостей"
	@echo "  run_*            - Запуск отдельных программ"
	@echo "  debug            - Сборка с отладочной информацией"
	@echo "  analyze          - Статический анализ кода"
	@echo "  valgrind_check   - Проверка утечек памяти"
	@echo "  profile          - Сборка для профилирования"
	@echo "  package          - Создание архива для сдачи"

# Файлы для отслеживания
.PHONY: all clean test benchmark demo check_deps install_deps debug analyze valgrind_check profile package help run_memory_info run_io_benchmark run_profiler clean_cache monitor test_files

# Автоматические зависимости
*.o: *.c
$(TARGETS): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)