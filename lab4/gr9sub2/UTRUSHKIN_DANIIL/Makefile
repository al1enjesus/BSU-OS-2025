# Компилятор
CC = gcc
# Флаги компиляции
CFLAGS = -Wall -Wextra -O2 -std=c99
# Все цели (исполняемые файлы)
TARGETS = memory_analysis mmap_vs_read page_faults memory_profiler

# Основная цель - компилирует всё
all: $(TARGETS)

# Правило для memory_analysis
memory_analysis: src/memory_analysis.c
	$(CC) $(CFLAGS) -o $@ $^

# Правило для mmap_vs_read
mmap_vs_read: src/mmap_vs_read.c
	$(CC) $(CFLAGS) -o $@ $^

# Правило для page_faults
page_faults: src/page_faults.c
	$(CC) $(CFLAGS) -o $@ $^

# Правило для memory_profiler
memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $^

# Очистка
clean:
	rm -f $(TARGETS) *.bin testfile.bin

# Запуск анализа памяти
run_analysis: memory_analysis
	./memory_analysis

# Псевдоцели
.PHONY: all clean run_analysis
