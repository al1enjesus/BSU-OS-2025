CC = gcc
CFLAGS = -Wall -Wextra -O2

all: memory_info io_benchmark page_faults_demo 

memory_info: src/memory_info.c
	$(CC) $(CFLAGS) -o memory_info src/memory_info.c

io_benchmark: src/io_benchmark.c
	$(CC) $(CFLAGS) -o io_benchmark src/io_benchmark.c

page_faults_demo: src/page_faults_demo.c
	$(CC) $(CFLAGS) -o page_faults_demo src/page_faults_demo.c

memory_profiler: src/memory_profiler.c
	$(CC) $(CFLAGS) -o memory_profiler src/memory_profiler.c


clean:
    rm -f memory_info io_benchmark page_faults_demo memory_profiler

