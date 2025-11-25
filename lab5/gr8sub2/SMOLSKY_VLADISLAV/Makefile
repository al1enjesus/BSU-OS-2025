# Имя текущей директории (для out-of-tree сборки)
PWD := $(shell pwd)

# Путь к исходникам ядра
KDIR := /lib/modules/$(shell uname -r)/build

# Список модулей для сборки
obj-m += src/hello_module.o
obj-m += src/proc_module.o
obj-m += src/sys_stat_module.o

all:
	@echo "Building kernel modules..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "✓ Modules built successfully!"
	@echo "  To load: sudo insmod <module>.ko"
	@echo "  To view logs: dmesg | tail"
	@echo ""

clean:
	@echo "Cleaning up..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions
	@echo "✓ Cleanup complete"