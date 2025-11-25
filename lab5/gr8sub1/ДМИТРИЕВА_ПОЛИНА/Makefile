
ifneq ($(KERNELRELEASE),)

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

hello_module-objs   := src/hello_module.o
proc_module-objs    := src/proc_module.o
chardev_module-objs := src/chardev_module.o

# дополнительные флаги компиляции
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

else

# ──────────────────────────────────────────────────────────────
#  обычный make из каталога лабораторной
# ──────────────────────────────────────────────────────────────

# путь к исходникам ядра
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

.DEFAULT_GOAL := all

.PHONY: all clean load unload info check test-hello test-proc test-chardev help

all:
	@echo "Building kernel modules in $(PWD)..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo "✓ Modules built:"
	@ls -1 *.ko 2>/dev/null || echo "  (no .ko files found)"
	@echo ""

clean:
	@echo "Cleaning..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions
	@echo "✓ Cleanup complete"

# Загрузка / выгрузка модулей
load:
	@if [ -z "$(MODULE)" ]; then \
		echo "Usage: make load MODULE=<module_name>"; \
		exit 1; \
	fi; \
	if [ ! -f "$(MODULE).ko" ]; then \
		echo "Error: $(MODULE).ko not found. Run 'make' first."; \
		exit 1; \
	fi; \
	echo "Loading $(MODULE).ko..."; \
	sudo insmod "$(MODULE).ko" && \
	echo "✓ Loaded. Use 'sudo dmesg | tail' to see output."

unload:
	@if [ -z "$(MODULE)" ]; then \
		echo "Usage: make unload MODULE=<module_name>"; \
		exit 1; \
	fi; \
	echo "Unloading $(MODULE)..."; \
	sudo rmmod "$(MODULE)" && \
	echo "✓ Unloaded. Use 'sudo dmesg | tail' to see output."

# Небольшая инфа
info:
	@echo "=== Built modules ==="
	@ls -lh *.ko 2>/dev/null || echo "No .ko files (run 'make')."
	@echo ""
	@echo "=== Loaded lab5 modules ==="
	@lsmod | head -1
	@lsmod | grep -E "hello_module|proc_module|chardev_module" || \
	echo "No lab5 modules loaded."

check:
	@echo "=== Environment check ==="
	@echo -n "Kernel version: "; uname -r
	@echo -n "Kernel headers: "
	@if [ -d "$(KERNEL_DIR)" ]; then \
		echo "✓ Found at $(KERNEL_DIR)"; \
	else \
		echo "✗ NOT FOUND"; \
		echo "  Install: sudo apt install linux-headers-$$(uname -r)"; \
	fi

# Быстрые тесты
test-hello: all
	@echo "=== Testing hello_module ==="
	-@sudo rmmod hello_module 2>/dev/null || true
	sudo insmod hello_module.ko
	@echo "dmesg (load):"
	@sudo dmesg | tail -5
	sudo rmmod hello_module
	@echo "dmesg (unload):"
	@sudo dmesg | tail -5

test-proc: all
	@echo "=== Testing proc_module ==="
	-@sudo rmmod proc_module 2>/dev/null || true
	sudo insmod proc_module.ko
	@echo "cat /proc/student_info:"
	@cat /proc/student_info || echo "File not found!"
	@echo ""
	@cat /proc/student_info || true
	sudo rmmod proc_module
	@echo "dmesg:"
	@sudo dmesg | tail -5

test-chardev: all
	@echo "=== Testing chardev_module ==="
	-@sudo rmmod chardev_module 2>/dev/null || true
	-@sudo rm /dev/mychardev 2>/dev/null || true
	sudo insmod chardev_module.ko
	@MAJOR=$$(sudo dmesg | \
	grep "chardev: Registered with major number" | \
	tail -1 | grep -o "number [0-9]*" | awk '{print $$2}'); \
	if [ -n "$$MAJOR" ]; then \
		echo "Creating /dev/mychardev with major $$MAJOR..."; \
		sudo mknod /dev/mychardev c $$MAJOR 0; \
		sudo chmod 666 /dev/mychardev; \
		echo "Hello" > /dev/mychardev; \
		echo "cat /dev/mychardev:"; \
		cat /dev/mychardev; \
		sudo rm /dev/mychardev; \
	else \
		echo "FAILED: could not detect major number (check dmesg)"; \
	fi
	sudo rmmod chardev_module
	@echo "dmesg:"
	@sudo dmesg | tail -10

help:
	@echo "Targets:"
	@echo "  make              - build all modules"
	@echo "  make clean        - clean build"
	@echo "  make load MODULE=hello_module"
	@echo "  make unload MODULE=hello_module"
	@echo "  make test-hello / test-proc / test-chardev"

endif
