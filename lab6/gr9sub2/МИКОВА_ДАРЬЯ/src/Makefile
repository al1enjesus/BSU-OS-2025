# Makefile для FUSE File Systems Lab 6 - Вариант 1

CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_DEFAULT_SOURCE
FUSE_CFLAGS = `pkgconf --cflags fuse3`
FUSE_LIBS = `pkgconf --libs fuse3`
LIBS = -pthread $(FUSE_LIBS)

# Целевые программы
TARGETS = myfuse archive_fs monitoring_fs benchmark

# Исходные файлы
SOURCES_A = A.c
SOURCES_B = B.c
SOURCES_C = C.c
SOURCES_BENCH = benchmark.c

# Объектные файлы
OBJS_A = $(SOURCES_A:.c=.o)
OBJS_B = $(SOURCES_B:.c=.o)
OBJS_C = $(SOURCES_C:.c=.o)
OBJS_BENCH = $(SOURCES_BENCH:.c=.o)

.PHONY: all clean help install

all: $(TARGETS)

# Задание A
myfuse: $(OBJS_A)
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) $^ $(LIBS) -o $@

# Задание B
archive_fs: $(OBJS_B)
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) $^ $(LIBS) -o $@

# Задание C
monitoring_fs: $(OBJS_C)
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) $^ $(LIBS) -o $@

# Benchmark
benchmark: $(OBJS_BENCH)
	$(CC) $(CFLAGS) -O3 $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) $(OBJS_A) $(OBJS_B) $(OBJS_C) $(OBJS_BENCH)
	rm -f *.o core

help:
	@echo "FUSE File Systems Lab 6 - Вариант 1"
	@echo ""
	@echo "Targets:"
	@echo "  make                - Compile all"
	@echo "  make myfuse         - Compile a"
	@echo "  make archive_fs     - Compile b"
	@echo "  make monitoring_fs  - Compile С"
	@echo "  make benchmark      - Compile benchmark program"
	@echo "  make clean          - Remove compiled files"
	@echo ""
	@echo "Usage:"
	@echo "  mkdir -p /tmp/source /tmp/mnt_a /tmp/mnt_b /tmp/mnt_c"
	@echo ""
	@echo "  # Задание A"
	@echo "  ./myfuse /tmp/source /tmp/mnt_a -f"
	@echo ""
	@echo "  # Задание B"
	@echo "  ./archive_fs /tmp/archive.tar /tmp/mnt_b -f"
	@echo ""
	@echo "  # Задание C"
	@echo "  ./monitoring_fs /tmp/source /tmp/mnt_c -f"
	@echo ""

install: all
	sudo mkdir -p /mnt/fuse_a /mnt/fuse_b /mnt/fuse_c
	@echo "Installation complete. Mount points created."

# Benchmark targets
.PHONY: bench bench-setup bench-clean

bench-setup:
	mkdir -p /tmp/fuse_bench /tmp/fuse_mount /tmp/fuse_bench_ext4

bench: all bench-setup
	./benchmark

bench-clean:
	fusermount -uz /tmp/fuse_mount 2>/dev/null || true
	rm -rf /tmp/fuse_bench /tmp/fuse_mount /tmp/fuse_bench_ext4

