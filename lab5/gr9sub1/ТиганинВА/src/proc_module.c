obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	@echo "Building all kernel modules..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

hello_module.ko: hello_module.c
	@echo "Building hello_module..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) hello_module.ko

proc_module.ko: proc_module.c
	@echo "Building proc_module..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) proc_module.ko

chardev_module.ko: chardev_module.c
	@echo "Building chardev_module..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) chardev_module.ko

clean:
	@echo "Cleaning up..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions

.PHONY: all clean