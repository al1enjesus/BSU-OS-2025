CC = gcc
CFLAGS = -O2 -std=gnu11 -Wall
SRC_DIR = .
BIN_DIR = .

ALL = memory_profiler file_analyzer pagefaults

.PHONY: all clean runA runB runC testfile

all: $(ALL)

memory_profiler: $(SRC_DIR)/memory_profiler.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<

file_analyzer: $(SRC_DIR)/file_analyzer.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<

pagefaults: $(SRC_DIR)/pagefaults.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<

# Создать тестовый файл 100 MB (уже есть)
testfile.bin:
	dd if=/dev/urandom of=testfile.bin bs=1M count=100

# Быстрый запуск задания A
runA: memory_profiler
 	@echo "Запуск memory_profiler (терминал 1) остальное в	другом терминале"
 	./memory_profiler

# Запуск задания B (создаёт testfile, чистит кэш и запускает программу) кеша--sudo
runB: file_analyzer testfile.bin
	@echo
	@echo
runB:
	@echo "Перед запуском очистите кэш вручную:"
	@echo "sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'"
	./file_analyzer
# Запуск задания C
runC: pagefaults
	./pagefaults

clean:
	rm -f $(BIN_DIR)/memory_profiler $(BIN_DIR)/file_analyzer $(BIN_DIR)/pagefaults testfile.bin

