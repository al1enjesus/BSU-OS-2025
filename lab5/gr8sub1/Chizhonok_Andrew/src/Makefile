obj-m += hello_module.o 
obj-m += proc_module.o  
obj-m += chardev_module.o

all:
	@echo "Checking system limits..."
	@ulimit -n
	@echo "Building modules..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
