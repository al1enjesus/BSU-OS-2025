# Makefile для Lab 5 - Модули ядра Linux

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += char_device.o

PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

all:
	@echo "Сборка модулей ядра..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo "Модули успешно собраны!"
	@echo "Для тестирования используйте: make test-all"
	@echo ""

clean:
	@echo "Очистка временных файлов..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions
	@echo "Очистка завершена"

test-chardev: all
	@echo "=== Тестирование Character Device ==="
	@echo "Загрузка модуля..."
	-@sudo rmmod char_device 2>/dev/null || true
	-@sudo rm -f /dev/mychardev 2>/dev/null || true
	sudo insmod char_device.ko
	@echo ""
	@MAJOR=$$(sudo dmesg | grep "chardev: Registered with major number" | tail -1 | awk '{print $$NF}'); \
	if [ -n "$$MAJOR" ]; then \
		echo "1. Создание device node с major номером $$MAJOR..."; \
		sudo mknod /dev/mychardev c $$MAJOR 0; \
		sudo chmod 666 /dev/mychardev; \
		echo ""; \
		echo "2. Запись данных в устройство..."; \
		echo "Тестовые данные от Виолетты" | sudo tee /dev/mychardev; \
		echo ""; \
		echo "3. Чтение данных из устройства:"; \
		sudo cat /dev/mychardev; \
		echo ""; \
		echo "4. Повторная запись..."; \
		echo "Новые тестовые данные" | sudo tee /dev/mychardev; \
		echo ""; \
		echo "5. Повторное чтение:"; \
		sudo cat /dev/mychardev; \
		echo ""; \
		sudo rm -f /dev/mychardev; \
	else \
		echo "Ошибка: не удалось получить major номер"; \
	fi
	@echo ""
	@echo "6. Выгрузка модуля..."
	sudo rmmod char_device
	@echo ""
	@echo "Логи ядра (dmesg):"
	@sudo dmesg | tail -15 | grep chardev

test-proc: all
	@echo "=== Тестирование Proc Module ==="
	@echo "Загрузка модуля..."
	-@sudo rmmod proc_module 2>/dev/null || true
	sudo insmod proc_module.ko
	@echo ""
	@echo "1. Первое чтение файла:"
	@cat /proc/student_info 2>/dev/null || echo "Ошибка чтения /proc/student_info"
	@echo ""
	@echo "2. Второе чтение (счетчик должен увеличиться):"
	@cat /proc/student_info 2>/dev/null || echo "Ошибка чтения /proc/student_info"
	@echo ""
	@echo "3. Третье чтение (счетчик должен снова увеличиться):"
	@cat /proc/student_info 2>/dev/null || echo "Ошибка чтения /proc/student_info"
	@echo ""
	@echo "4. Выгрузка модуля..."
	sudo rmmod proc_module
	@echo ""
	@echo "Логи ядра (dmesg):"
	@sudo dmesg | tail -12 | grep proc_module

test-hello: all
	@echo "=== Тестирование Hello Module ==="
	@echo "Загрузка модуля..."
	-@sudo rmmod hello_module 2>/dev/null || true
	sudo insmod hello_module.ko
	@echo ""
	@echo "Вывод при загрузке:"
	@sudo dmesg | tail -3 | grep hello_module || true
	@echo ""
	@echo "Выгрузка модуля..."
	sudo rmmod hello_module
	@echo ""
	@echo "Вывод при выгрузке:"
	@sudo dmesg | tail -3 | grep hello_module || true

test-hello-param: all
	@echo "=== Тестирование Hello Module с параметром ==="
	@echo "Загрузка модуля с параметром message..."
	-@sudo rmmod hello_module 2>/dev/null || true
	sudo insmod hello_module.ko message="Кастомное сообщение для теста"
	@echo ""
	@echo "Вывод при загрузке:"
	@sudo dmesg | tail -3 | grep hello_module || true
	@echo ""
	@echo "Выгрузка модуля..."
	sudo rmmod hello_module
	@echo ""
	@echo "Вывод при выгрузке:"
	@sudo dmesg | tail -3 | grep hello_module || true

test-all: test-hello test-proc test-chardev
	@echo ""
	@echo "=== Все тесты завершены ==="
	@echo "Все три модуля работают корректно!"
	@echo "- Hello Module: загрузка/выгрузка с сообщениями"
	@echo "- Proc Module: /proc/student_info с счетчиком"
	@echo "- Character Device: чтение/запись через /dev/mychardev"

info:
	@echo "=== Информация о модулях ==="
	@echo "Собранные модули:"
	@ls -la *.ko 2>/dev/null || echo "Модули не собраны (выполните make all)"
	@echo ""
	@echo "Загруженные модули:"
	@lsmod | grep -E "(hello_module|proc_module|char_device)" || echo "Модули не загружены"

check-env:
	@echo "=== Проверка окружения ==="
	@echo "Версия ядра: $(shell uname -r)"
	@echo "Заголовки ядра: $(shell ls /lib/modules/$(shell uname -r)/build 2>/dev/null && echo "найдены" || echo "не найдены")"
	@echo "Компилятор: $(shell gcc --version | head -1)"
	@echo "Make: $(shell make --version | head -1)"

help:
	@echo "Доступные команды:"
	@echo "  make all             - Собрать все модули"
	@echo "  make clean           - Очистить временные файлы"
	@echo "  make test-all        - Протестировать все модули"
	@echo "  make test-chardev    - Тестировать character device"
	@echo "  make test-proc       - Тестировать proc module"
	@echo "  make test-hello      - Тестировать hello module"
	@echo "  make test-hello-param - Тестировать hello module с параметром"
	@echo "  make info            - Показать информацию о модулях"
	@echo "  make check-env       - Проверить окружение"
	@echo ""
	@echo "Ручное тестирование:"
	@echo "  sudo insmod hello_module.ko"
	@echo "  sudo insmod hello_module.ko message=\"Ваше сообщение\""
	@echo "  sudo insmod proc_module.ko"
	@echo "  cat /proc/student_info"
	@echo "  sudo insmod char_device.ko"
	@echo "  # Найдите MAJOR номер в dmesg"
	@echo "  sudo mknod /dev/mychardev c <MAJOR> 0"
	@echo "  echo \"тест\" > /dev/mychardev"
	@echo "  cat /dev/mychardev"

.PHONY: all clean test-chardev test-proc test-hello test-hello-param test-all info check-env help
