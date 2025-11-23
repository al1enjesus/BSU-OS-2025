# Makefile for kernel modules

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target
all:
	@echo "Building kernel modules..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "Build complete!"

# Clean target
clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.mod.c Module.symvers modules.order
	@echo "Clean complete!"

# Install all modules
install:
	@echo "Installing modules..."
	sudo insmod hello_module.ko
	sudo insmod proc_module.ko
	sudo insmod chardev_module.ko
	@echo "Installation complete!"

# Uninstall all modules
uninstall:
	@echo "Uninstalling modules..."
	-sudo rmmod hello_module
	-sudo rmmod proc_module
	-sudo rmmod chardev_module
	@echo "Uninstall complete!"

# Test character device
test-chardev:
	@echo "Testing character device..."
	@if [ -f chardev_module.ko ]; then \
		sudo insmod chardev_module.ko; \
		MAJOR=$$(dmesg | grep "major number" | tail -1 | awk '{print $$NF}'); \
		echo "Detected major number: $$MAJOR"; \
		sudo mknod /dev/mychardev c $$MAJOR 0; \
		sudo chmod 666 /dev/mychardev; \
		echo "Character device created at /dev/mychardev"; \
	else \
		echo "chardev_module.ko not found, build first with 'make'"; \
	fi

# Clean character device
clean-chardev:
	-sudo rm -f /dev/mychardev
	@echo "Character device node removed"

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build all kernel modules"
	@echo "  clean        - Clean build files"
	@echo "  install      - Install all modules"
	@echo "  uninstall    - Uninstall all modules"
	@echo "  test-chardev - Test character device setup"
	@echo "  clean-chardev - Remove character device node"

.PHONY: all clean install uninstall test-chardev clean-chardev help
