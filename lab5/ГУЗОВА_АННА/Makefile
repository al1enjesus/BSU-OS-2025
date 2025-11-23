obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	@echo "Building kernel modules in $(PWD)..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
