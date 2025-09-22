# Компилятор
CC = gcc

# Флаги компиляции
CFLAGS = -Wall -Wextra -std=c11 -pedantic

# Имя исполняемого файла
TARGET = myprogram

# Исходные файлы
SRCS = prog.c

# Объектные файлы (автоматически из SRCS)
OBJS = $(SRCS:.c=.o)

# Правило по умолчанию
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Компиляция каждого .c файла в .o файл
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка собранных файлов
clean:
	rm -f $(OBJS) $(TARGET)

# Пересборка
re: clean all

# Установка (если нужно)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Не считать эти файлы за цели
.PHONY: all clean re install
