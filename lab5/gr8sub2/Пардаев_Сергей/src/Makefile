obj-m += hellomodule.o
obj-m += procmodule.o
obj-m += chardevmodule.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	sudo insmod hellomodule.ko
	sudo insmod procmodule.ko
	sudo insmod chardevmodule.ko

unload:
	sudo rmmod chardevmodule
	sudo rmmod procmodule
	sudo rmmod hellomodule

info:
	lsmod | grep module
	dmesg | tail -30

test-hello:
	sudo insmod hellomodule.ko
	dmesg | tail -5
	sudo rmmod hellomodule
	dmesg | tail -5

test-proc:
	sudo insmod procmodule.ko
	cat /proc/studentinfo
	sudo rmmod procmodule

test-chardev:
	sudo insmod chardevmodule.ko
	dmesg | grep chardev | tail -1
	sudo rmmod chardevmodule
