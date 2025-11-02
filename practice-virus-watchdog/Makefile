CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = watchdog
SOURCES = watchdog.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

.PHONY: clean install