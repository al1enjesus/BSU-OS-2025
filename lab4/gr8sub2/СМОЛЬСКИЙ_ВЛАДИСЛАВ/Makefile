# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Имена исполняемых файлов
TARGET1 = memory_info
TARGET2 = buf_vs_nobuf

# Директории
SRCDIR = src
BINDIR = bin

# Полные пути к исполняемым файлам
BINARY1 = $(BINDIR)/$(TARGET1)
BINARY2 = $(BINDIR)/$(TARGET2)

# Основные цели
all: $(BINARY1) $(BINARY2)

memory_info: $(BINARY1)

buf_vs_nobuf: $(BINARY2)

# Создание директорий если их нет
$(BINDIR):
	mkdir -p $(BINDIR)

# Правила сборки
$(BINARY1): $(SRCDIR)/$(TARGET1).c | $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/$(TARGET1).c -o $(BINARY1)

$(BINARY2): $(SRCDIR)/$(TARGET2).c | $(BINDIR)
	$(CC) $(CFLAGS) $(SRCDIR)/$(TARGET2).c -o $(BINARY2)

# Запуск программ
# run_memory_info: $(BINARY1)
# 	./$(BINARY1)

# Очистка
clean:
	rm -rf $(BINDIR)

.PHONY: all memory_info buf_vs_nobuf clean