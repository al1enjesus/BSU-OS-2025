# Указываем модуль при вызове: make MODULE=hello_module or MODULE=MODULE=student_info_module or make MODULE=chardev_module
MODULE ?= hello_module

obj-m := $(MODULE).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
