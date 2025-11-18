# Makefile для Lab 5 - Модули ядра Linux
obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "Building kernel modules..."
	@sudo make -C $(KERNEL_DIR) M=$(PWD) modules
	@echo "✓ Modules built successfully!"	
clean:
	@echo "Cleaning up..."
	@sudo make -C $(KERNEL_DIR) M=$(PWD) clean
	@sudo rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	@sudo rm -rf .tmp_versions
	@echo "✓ Cleanup complete"

test-hello: all
	@echo "=== Testing hello_module ==="
	@-sudo rmmod hello_module 2>/dev/null || true
	@echo "1. Loading module..."
	@sudo insmod hello_module.ko
	@echo "2. Checking dmesg..."
	@sudo sh -c "dmesg | tail -5 | grep hello_module"
	@echo "3. Unloading module..."
	@sudo rmmod hello_module
	@echo "4. Checking dmesg..."
	@sudo sh -c "dmesg | tail -5 | grep hello_module"
	@echo "✓ hello_module test completed"

test-hello-param: all
	@echo "=== Testing hello_module with parameter ==="
	@-sudo rmmod hello_module 2>/dev/null || true
	@echo "Loading with custom message..."
	@sudo insmod hello_module.ko message="Custom greeting from kernel!"
	@sudo sh -c "dmesg | tail -3 | grep hello_module"
	@sudo rmmod hello_module
	@sudo sh -c "dmesg | tail -3 | grep hello_module"

test-proc: all
	@echo "=== Testing proc_module ==="
	@-sudo rmmod proc_module 2>/dev/null || true
	@echo "1. Loading module..."
	@sudo insmod proc_module.ko
	@echo "2. Reading /proc/student_info:"
	@cat /proc/student_info 2>/dev/null || echo "   File not created"
	@echo "3. Reading again (counter should increase):"
	@cat /proc/student_info 2>/dev/null || echo "   File not created"
	@echo "4. Unloading module..."
	@sudo rmmod proc_module
	@echo "✓ proc_module test completed"	

test-chardev: all
	@echo "=== Testing chardev_module ==="
	@-sudo rmmod chardev_module 2>/dev/null || true
	@-sudo rm -f /dev/mychardev 2>/dev/null || true
	@echo "1. Loading module..."
	@sudo insmod chardev_module.ko
	@echo "2. Checking major number..."
	@MAJOR=$$(sudo sh -c "dmesg | grep 'Registered' | tail -1 | grep -o 'number [0-9]*' | awk '{print \$$2}'"); \
	if [ -n "$$MAJOR" ]; then \
	 echo "   Major number: $$MAJOR"; \
	 echo "3. Creating device node..."; \
	 sudo mknod /dev/mychardev c $$MAJOR 0; \
	 sudo chmod 666 /dev/mychardev; \
	 echo "4. Writing to device..."; \
	 echo "Hello Kernel World!" > /dev/mychardev; \
	 echo "5. Reading from device:"; \
	 cat /dev/mychardev; \
	 echo "6. Cleaning up device..."; \
	 sudo rm -f /dev/mychardev; \
	else \
	 echo "   Failed to get major number"; \
	fi
	@echo "7. Unloading module..."
	@sudo rmmod chardev_module
	@echo "✓ chardev_module test completed"

test-all: 
	@echo "Running all tests..."
	@make test-hello
	@make test-proc
	@make test-chardev
	@echo "All tests completed!"

.PHONY: all clean test-hello test-hello-param test-proc test-chardev test-all