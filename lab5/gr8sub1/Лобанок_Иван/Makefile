obj-m += src/hello_module.o
obj-m += src/proc_config_module.o
obj-m += src/sys_stats_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
