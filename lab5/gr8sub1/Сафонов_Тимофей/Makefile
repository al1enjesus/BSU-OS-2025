obj-m += hello_module.o
obj-m += proc_module.o  
obj-m += chardev_module.o

hello_module-objs := src/hello_module.o
proc_module-objs := src/proc_module.o
chardev_module-objs := src/chardev_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

logs:
	sudo dmesg | tail -5

clear-logs:
	sudo dmesg -c

.PHONY: all clean load unload logs clear-logs