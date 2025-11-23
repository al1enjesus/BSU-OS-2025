# Makefile для сборки модулей ядра Linux для ядра 6.14.0-32-generic

# Текущая рабочая директория (где лежит Makefile и исходники)
PWD := $(shell pwd)

# Каталог с заголовками ядра - корректно для вашей версии ядра
KERNEL_DIR := /lib/modules/$(shell uname -r)/build

# Список модулей для сборки (.c файлы должны быть в этой же директории)
obj-m := hello_module.o proc_module.o chardev_module.o

# Флаги компиляции (если надо, можно добавить дополнительные)
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

# Цель сборки всех модулей
all:
	@echo "Building kernel modules for kernel $(shell uname -r)..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo "✔️ Modules built successfully."

# Очистка всех временных файлов сборки
clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions
	@echo "✔️ Cleanup complete."

# Загрузка модуля
load:
ifndef MODULE
	$(error MODULE не задан, пример: make load MODULE=hello_module)
endif
	@echo "Loading module $(MODULE).ko..."
	sudo insmod $(MODULE).ko
	@echo "✔️ Loaded. Проверьте вывод: dmesg | tail"

# Выгрузка модуля
unload:
ifndef MODULE
	$(error MODULE не задан, пример: make unload MODULE=hello_module)
endif
	@echo "Unloading module $(MODULE)..."
	sudo rmmod $(MODULE)
	@echo "✔️ Unloaded. Проверьте вывод: dmesg | tail"

.PHONY: all clean load unload
