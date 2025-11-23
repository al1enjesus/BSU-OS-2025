obj-m += hello_module.o
obj-m += proc_module.o
obj-m += sys_stats_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install-hello:
	sudo insmod hello_module.ko

install-proc:
	sudo insmod proc_module.ko

install-stats:
	sudo insmod sys_stats_module.ko

uninstall-all:
	-sudo rmmod hello_module
	-sudo rmmod proc_module
	-sudo rmmod sys_stats_module
