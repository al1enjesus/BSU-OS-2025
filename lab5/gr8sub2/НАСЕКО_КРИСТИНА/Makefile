# Makefile для Варианта 2
PWD := $(shell pwd)
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Файлы модулей для Варианта 2
obj-m += hello_module.o       # Задание A
obj-m += config_module.o      # Задание B (/proc с записью)
obj-m += stats_module.o       # Задание C (/proc статистика)

# Флаги для чистоты кода и подавления предупреждений стандарта C90
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	@echo "Building Variant 2 modules..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo "✓ Build complete."

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions

# Тесты
test-all:
	# Тест A
	sudo insmod hello_module.ko message="Test" && \
	sudo rmmod hello_module && \
	# Тест B
	sudo insmod config_module.ko && \
	echo "TestConfig" | sudo tee /proc/my_config && \
	cat /proc/my_config && \
	sudo rmmod config_module && \
	# Тест C
	sudo insmod stats_module.ko && \
	cat /proc/sys_stats && \
	sudo rmmod stats_module && \
	dmesg | tail -10
