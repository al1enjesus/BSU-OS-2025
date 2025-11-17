
obj-m += hello_module.o
obj-m += proc_module.o
obj-m += stats_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.ko *.mod* *.o *.symvers *.order .*.cmd
	rm -rf .tmp_versions modules.order Module.symvers

install:
	sudo insmod hello_module.ko
	sudo insmod proc_module.ko
	sudo insmod stats_module.ko

uninstall:
	sudo rmmod stats_module proc_module hello_module

.PHONY: all clean install uninstall

