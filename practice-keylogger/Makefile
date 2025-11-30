PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

obj-m += keylogger_module.o

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd

load:
	sudo insmod keylogger_module.ko

unload:
	sudo rmmod keylogger_module
