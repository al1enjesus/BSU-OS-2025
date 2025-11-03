# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Имена исполняемых файлов
TARGET1 = memory_info
TARGET2 = io_benchmark
TARGET3 = memory_profiler

# Директории
SRCDIR = src
BINDIR = bin

# Полные пути к исполняемым файлам
BINARY1 = $(BINDIR)/$(TARGET1)
BINARY2 = $(BINDIR)/$(TARGET2)
BINARY3 = $(BINDIR)/$(TARGET3)

# Основные цели
all: $(BINARY1) $(BINARY2)

memory_info: $(BINARY1)
io_benchmark: $(BINARY2)
memory_profiler: $(BINARY3)

# Создание директорий если их нет
$(BINDIR):
	mkdir -p $(BINDIR)

# Правила сборки
$(BINARY1): $(SRCDIR)/$(TARGET1).c | $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/$(TARGET1).c -o $(BINARY1)

$(BINARY2): $(SRCDIR)/$(TARGET2).c | $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/$(TARGET2).c -o $(BINARY2)

$(BINARY3): $(SRCDIR)/$(TARGET3).c | $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/$(TARGET3).c -o $(BINARY3)

# Очистка
clean:
	rm -rf $(BINDIR)

.PHONY: all memory_info io_benchmark clean