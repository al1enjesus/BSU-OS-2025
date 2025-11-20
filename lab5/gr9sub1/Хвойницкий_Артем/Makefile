obj-m += hello_module.o
obj-m += my_config_module.o
obj-m += sys_stats_module.o
obj-m += char_device.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
