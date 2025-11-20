# Makefile для Лабораторной работы 4

# Компилятор и флаги
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2

# Цели
TARGETS = src/memory_info src/io_benchmark src/io_probe src/memory_profiler
PYTHON_SCRIPTS = src/plot_io_bench.py

# Директории
BENCH_LOGS = bench_logs
BENCH_PLOTS = bench_plots

.PHONY: all clean bench bench-full profile strace plots help

all: $(TARGETS)

# Сборка программ
src/memory_info: src/memory_info.c
	$(CC) $(CFLAGS) -o $@ $<

src/io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

src/io_probe: src/io_probe.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

src/memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

# Создание директорий
$(BENCH_LOGS):
	mkdir -p $(BENCH_LOGS)

$(BENCH_PLOTS):
	mkdir -p $(BENCH_PLOTS)

# Задание A: Анализ памяти
memory-test: src/memory_info
	@echo "=== Задание A: Анализ памяти ==="
	./src/memory_info </dev/null & PID=$$!; \
	ps -o pid,vsz,rss -p $$PID; \
	cat /proc/$$PID/status | grep ^Vm; \
	wait

# Задание B: Бенчмарк I/O
bench: src/io_benchmark $(BENCH_LOGS)
	@echo "=== Задание B: Бенчмарк I/O (100MB) ==="
	./src/io_benchmark --size 100 | tee "$(BENCH_LOGS)/run_100MB_$$(date +%F_%H-%M-%S).log"

bench-full: src/io_benchmark $(BENCH_LOGS)
	@echo "=== Задание B: Полный бенчмарк I/O ==="
	@echo "Запуск на 100MB..."
	./src/io_benchmark --size 100 | tee "$(BENCH_LOGS)/run_100MB_$$(date +%F_%H-%M-%S).log"
	@echo "Запуск на 500MB..."  
	./src/io_benchmark --size 500 | tee "$(BENCH_LOGS)/run_500MB_$$(date +%F_%H-%M-%S).log"
	@echo "Запуск для анализа буферов..."
	./src/io_benchmark --size 100 | tee "$(BENCH_LOGS)/bufs_100MB_$$(date +%F_%H-%M-%S).log"

# Strace анализ
strace: src/io_benchmark
	@echo "=== Strace анализ ==="
	strace -c -e trace=open,read,write,close,fsync,fdatasync ./src/io_benchmark --size 100

# Генерация графиков
plots: $(BENCH_PLOTS)
	@echo "=== Генерация графиков ==="
	@if [ -f buffers_vs_throughput.csv ] && [ -f methods_vs_throughput.csv ]; then \
		python3 src/plot_io_bench.py --outdir $(BENCH_PLOTS); \
		echo "Графики сохранены в $(BENCH_PLOTS)/"; \
	else \
		echo "CSV файлы не найдены. Сначала выполните 'make bench-full'"; \
	fi

# Создание CSV файлов
csv: $(BENCH_LOGS)
	@echo "=== Создание CSV файлов ==="
	@echo "Создание buffers_vs_throughput.csv..."
	@awk '\
	/^=== write\(\) with buffer=/ { \
	  if (match($$0,/buffer=([0-9]+) bytes/,m)) buf=m[1] \
	} \
	/^Throughput:/ { \
	  thr=$$2; gsub(",",".",thr); \
	  if (buf!="") print buf","thr \
	}' $(BENCH_LOGS)/bufs_*.log | sort -n > buffers_vs_throughput.csv
	
	@echo "Создание methods_vs_throughput.csv..."
	@awk '\
	/^=== fwrite/   {m="fwrite"} \
	/^=== write\(/  {m="write"} \
	/^=== mmap\(\)/ {m="mmap"} \
	/^Throughput:/  {thr=$$2; gsub(",",".",thr); print m","thr} \
	' $(BENCH_LOGS)/run_500MB_*.log > methods_vs_throughput.csv
	
	@echo "CSV файлы созданы: buffers_vs_throughput.csv, methods_vs_throughput.csv"

# Задание C: Дисковый I/O
io-probe: src/io_probe
	@echo "=== Задание C: Дисковый I/O ==="
	./src/run_mon.sh ./src/io_probe --file io.bin --size-mb 2000 --mode write --bs 1048576

# Memory Profiler
profile: src/memory_profiler
	@echo "=== Memory Profiler ==="
	@echo "Примеры использования:"
	@echo "  ./src/memory_profiler \$$$ --map"
	@echo "  ./src/memory_profiler \$$$ --compare \$$(pgrep -n chrome)"
	@echo "  timeout 12s ./src/memory_profiler \$$$ --watch 1"

# Полный прогон всех тестов
full-test: all bench-full csv plots
	@echo "=== Все тесты завершены ==="
	@echo "Результаты в: $(BENCH_LOGS)/, $(BENCH_PLOTS)/"
	@echo "CSV файлы: buffers_vs_throughput.csv, methods_vs_throughput.csv"

# Очистка
clean:
	rm -f $(TARGETS)
	rm -f io.bin
	rm -f *.csv
	rm -rf $(BENCH_LOGS) $(BENCH_PLOTS)

# Справка
help:
	@echo "Доступные цели:"
	@echo "  all          - Собрать все программы"
	@echo "  memory-test  - Задание A: тест памяти"
	@echo "  bench        - Задание B: быстрый бенчмарк (100MB)"
	@echo "  bench-full   - Задание B: полный бенчмарк (100MB + 500MB)"
	@echo "  strace       - Strace анализ"
	@echo "  csv          - Создать CSV файлы из логов"
	@echo "  plots        - Сгенерировать графики"
	@echo "  io-probe     - Задание C: дисковый I/O"
	@echo "  profile      - Memory profiler"
	@echo "  full-test    - Полный прогон всех тестов"
	@echo "  clean        - Очистка"
