# Makefile для Lab 5 - Модули ядра Linux

PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Список модулей для сборки
obj-m += src/hello_module.o
obj-m += src/proc_module.o
obj-m += src/chardev_module.o

# Флаги компиляции
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

# Основная цель - сборка всех модулей
all:
	@echo "Building kernel modules..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo "✓ Modules built successfully!"

# Очистка
clean:
	@echo "Cleaning up..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f src/*.o src/*.ko src/*.mod.c src/*.mod src/*.symvers src/*.order src/.*.cmd
	@echo "✓ Cleanup complete"

# Загрузка модуля
load:
	@if [ -z "$(MODULE)" ]; then \
		echo "Error: Please specify MODULE=<module_name>"; \
		echo "Example: make load MODULE=hello_module"; \
		exit 1; \
	fi
	@if [ ! -f src/$(MODULE).ko ]; then \
		echo "Error: $(MODULE).ko not found. Run 'make' first."; \
		exit 1; \
	fi
	@echo "Loading $(MODULE).ko..."
	sudo insmod src/$(MODULE).ko
	@echo "✓ Module loaded. Check 'dmesg | tail' for output"

# Выгрузка модуля
unload:
	@if [ -z "$(MODULE)" ]; then \
		echo "Error: Please specify MODULE=<module_name>"; \
		echo "Example: make unload MODULE=hello_module"; \
		exit 1; \
	fi
	@echo "Unloading $(MODULE)..."
	sudo rmmod $(MODULE)
	@echo "✓ Module unloaded. Check 'dmesg | tail' for output"

# Информация о модулях
info:
	@echo "=== Loaded Modules ==="
	@lsmod | head -1
	@lsmod | grep -E "(hello_module|proc_module|chardev_module)" || echo "No lab5 modules loaded"
	@echo ""
	@echo "=== Built Modules ==="
	@ls -lh src/*.ko 2>/dev/null || echo "No modules built yet (run 'make')"

# Тест hello_module
test-hello: all
	@echo "=== Testing hello_module ==="
	-@sudo rmmod hello_module 2>/dev/null || true
	@echo "Loading module..."
	sudo insmod src/hello_module.ko
	@echo "Output:"
	@dmesg | tail -3
	@echo "Unloading module..."
	sudo rmmod hello_module
	@echo "Output:"
	@dmesg | tail -3

# Тест proc_module
test-proc: all
	@echo "=== Testing proc_module ==="
	-@sudo rmmod proc_module 2>/dev/null || true
	@echo "Loading module..."
	sudo insmod src/proc_module.ko
	@echo "Reading /proc/student_info:"
	@cat /proc/student_info 2>/dev/null || echo "File not created (check dmesg)"
	@echo "Unloading module..."
	sudo rmmod proc_module

# Тест chardev_module
test-chardev: all
	@echo "=== Testing chardev_module ==="
	-@sudo rmmod chardev_module 2>/dev/null || true
	-@sudo rm -f /dev/mychardev 2>/dev/null || true
	@echo "Loading module..."
	sudo insmod src/chardev_module.ko
	@sleep 1
	@MAJOR=$$(dmesg | grep "Registered with major number" | tail -1 | grep -o "[0-9]*" | head -1); \
	if [ -n "$$MAJOR" ]; then \
		echo "Creating device node with major $$MAJOR..."; \
		sudo mknod /dev/mychardev c $$MAJOR 0; \
		sudo chmod 666 /dev/mychardev; \
		echo "Testing device..."; \
		echo "Test data" | sudo tee /dev/mychardev > /dev/null; \
		echo "Reading from device:"; \
		sudo cat /dev/mychardev; \
		sudo rm -f /dev/mychardev; \
	else \
		echo "Failed to get major number"; \
	fi
	@echo "Unloading module..."
	sudo rmmod chardev_module

.PHONY: all clean load unload info test-hello test-proc test-chardev
