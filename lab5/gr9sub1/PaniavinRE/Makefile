PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions

load:
	@if [ -z "$(MODULE)" ]; then echo "Usage: make load MODULE=name"; exit 1; fi
	sudo insmod $(MODULE).ko

unload:
	@if [ -z "$(MODULE)" ]; then echo "Usage: make unload MODULE=name"; exit 1; fi
	sudo rmmod $(MODULE)

test-all: all
	make test-hello
	make test-proc
	make test-chardev

test-hello:
	@echo "=== Testing hello_module ==="
	-sudo rmmod hello_module 2>/dev/null || true
	sudo insmod hello_module.ko
	sudo insmod hello_module.ko message="Привет от Ромчика!"
	dmesg | tail -5
	sudo rmmod hello_module

test-proc:
	@echo "=== Testing proc_module (Variant 1) ==="
	-sudo rmmod proc_module 2>/dev/null || true
	sudo insmod proc_module.ko
	cat /proc/student_info
	cat /proc/student_info
	sudo rmmod proc_module

test-chardev:
	@echo "=== Testing chardev_module ==="
	-sudo rmmod chardev_module 2>/dev/null || true
	-sudo rm /dev/mychardev 2>/dev/null || true
	sudo insmod chardev_module.ko
	@MAJOR=$$(dmesg | tail -20 | grep "major number" | tail -1 | awk '{print $$NF}'); \
	if [ -n "$$MAJOR" ]; then \
		sudo mknod /dev/mychardev c $$MAJOR 0; \
		sudo chmod 666 /dev/mychardev; \
		echo "Test from Variant 1" > /dev/mychardev; \
		echo "Reading:"; cat /dev/mychardev; \
		sudo rm /dev/mychardev; \
	else \
		echo "Не удалось получить major номер"; \
	fi
	sudo rmmod chardev_module

.PHONY: all clean test-all
