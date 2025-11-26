CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11


all: task_A task_B task_C memory_profiler

task_A:
	$(CC) $(CFLAGS) A_task/task_A.c -o A_task/A

task_B:
	$(CC) $(CFLAGS) B_task/task_B.c -o B_task/B

task_C:
	$(CC) $(CFLAGS) C_task/task_C.c -o C_task/C

memory_profiler:
	$(CC) $(CFLAGS) memory_prof/mem_prof.c -o memory_prof/memory_profiler

run_task_A: task_A
	./A_task/A

run_task_B: task_B
	./B_task/B testfile --create-file

run_task_C: task_C
	./C_task/C

run_memory_profiler: memory_profiler
	./memory_prof/memory_profiler $$$$

clean_A:
	rm -f A_task/A

clean_C:
	rm -f C_task/C

clean_B:
	rm -f B_task/B

clean_memory_profiler:
	rm -f memory_prof/memory_profiler 100
	

clean:
	$(MAKE) clean_A
	$(MAKE) clean_B
	$(MAKE) clean_C
	$(MAKE) clean_memory_profiler

.PHONY: all clean clean_A clean_B clean_C clean_memory_profiler run_task_A run_task_B run_task_C run_memory_profiler