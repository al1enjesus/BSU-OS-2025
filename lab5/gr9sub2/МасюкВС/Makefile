KERNEL_DIR := /lib/modules/$(shell uname -r)/build

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += sys_stats_module.o

SRC_DIR = src

hello_module-objs := $(SRC_DIR)/hello_module.o
proc_module-objs := $(SRC_DIR)/proc_module.o
sys_stats_module-objs := $(SRC_DIR)/sys_stats_module.o

all:
	@echo "Building kernel modules"
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo " Modules built successfully!"

clean:
	@echo "Cleaning up..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd
	rm -rf .tmp_versions
	@echo " Cleanup complete"

load:
	@if [ -z "$(MODULE)" ]; then \
		echo "Error: Please specify MODULE=<module_name>"; \
		echo "Example: make load MODULE=hello_module"; \
		exit 1; \
	fi
	@if [ ! -f $(MODULE).ko ]; then \
		echo "Error: $(MODULE).ko not found. Run 'make' first."; \
		exit 1; \
	fi
	@echo "Loading $(MODULE).ko..."
	sudo insmod $(MODULE).ko
	@echo " Module loaded"

unload:
	@if [ -z "$(MODULE)" ]; then \
		echo "Error: Please specify MODULE=<module_name>"; \
		echo "Example: make unload MODULE=hello_module"; \
		exit 1; \
	fi
	@echo "Unloading $(MODULE)..."
	sudo rmmod $(MODULE)
	@echo " Module unloaded"

help:
	@echo "  make                  - Build all modules from src/"
	@echo "  make clean            - Remove built modules"
	@echo "  make load MODULE=<name>    - Load a module"
	@echo "  make unload MODULE=<name>  - Unload a module"

.PHONY: all clean load unload info test-hello test-proc test-stats check help
