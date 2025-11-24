obj-m += hello_module.o 
obj-m += student_info_module.o
obj-m += chardev_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
