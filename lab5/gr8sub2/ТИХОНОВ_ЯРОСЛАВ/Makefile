	KDIR ?= /lib/modules/$(shell uname -r)/build	
	PWD := $(shell pwd)

obj-m := hello_module.o proc_config.o proc_sys_stats.o

all: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
