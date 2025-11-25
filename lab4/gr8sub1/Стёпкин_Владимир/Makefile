CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGETS = memory_profiler file_analyzer page_faults

SRCS = memory_profiler.c file_analyzer.c page_faults.c

all: $(TARGETS)

memory_profiler: memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

file_analyzer: file_analyzer.c
	$(CC) $(CFLAGS) -o $@ $<

page_faults: page_faults.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

run: all
	@echo "Запуск memory_profiler"
	./memory_profiler
	@echo "Запуск file_analyzer"
	./file_analyzer
	@echo "Запуск page_faults"
	./page_faults
