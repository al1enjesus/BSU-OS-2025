obj-m += src/hello_module.o
obj-m += src/proc_module.o
obj-m += src/char_device.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install-hello:
	sudo insmod src/hello_module.ko

install-proc:
	sudo insmod src/proc_module.ko

install-chardev:
	sudo insmod src/char_device.ko

uninstall-hello:
	sudo rmmod hello_module

uninstall-proc:
	sudo rmmod proc_module

uninstall-chardev:
	sudo rmmod char_device

.PHONY: all clean install-hello install-proc install-chardev uninstall-hello uninstall-proc uninstall-chardev
