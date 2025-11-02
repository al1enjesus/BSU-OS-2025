CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
SRCDIR = src

# Все цели для компиляции
TARGETS = memory_analysis io_comparison page_faults_analysis memory_profiler file_analyzer

all: $(TARGETS)

memory_analysis: $(SRCDIR)/memory_analysis.c
	$(CC) $(CFLAGS) -o $@ $<

io_comparison: $(SRCDIR)/io_comparison.c
	$(CC) $(CFLAGS) -o $@ $<

page_faults_analysis: $(SRCDIR)/page_faults_analysis.c
	$(CC) $(CFLAGS) -o $@ $<

memory_profiler: $(SRCDIR)/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

file_analyzer: $(SRCDIR)/file_analyzer.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) testfile.bin

test: all
	@echo "=== Задание A: Анализ памяти ==="
	./memory_analysis || echo "Программа завершена"
	@echo "=== Задание B: Сравнение I/O ==="
	./io_comparison || echo "Программа завершена"
	@echo "=== Задание C: Page Faults ==="
	./page_faults_analysis || echo "Программа завершена"

.PHONY: all clean test
