obj-m += src/hello_module.o
obj-m += src/proc_module.o
obj-m += src/sys_stats_module.o

PWD := $(shell pwd)

all:
	@echo "Building kernel modules..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@echo ""
	@echo "✓ Modules built successfully!"
	@echo "  To load: sudo insmod <module>.ko"
	@echo "  To view logs: dmesg | tail"
	@echo ""

clean:
	@echo "Cleaning up..."
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	@echo "✓ Cleanup complete"

test-hello: all
	@echo "=== Testing hello_module ==="
	@echo "Loading module..."
	-@sudo rmmod hello_module 2>/dev/null || true
	$(eval PARAM := $(filter-out $@,$(MAKECMDGOALS)))
	@if [ -n "$(PARAM)" ]; then \
		sudo insmod src/hello_module.ko message="$(PARAM)"; \
	else \
		sudo insmod src/hello_module.ko; \
	fi
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5
	@echo ""
	@echo "Unloading module..."
	sudo rmmod hello_module
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5

test-proc: all
	@echo "=== Testing proc_module ==="
	@echo "Loading module..."
	-@sudo rmmod proc_module 2>/dev/null || true
	sudo insmod src/proc_module.ko;
	$(eval PARAM := $(filter-out $@,$(MAKECMDGOALS)))
	@if [ -n "$(PARAM)" ]; then \
		echo "Starting value of config: "; \
		cat /proc/my_config ; \
		echo "Writing $(PARAM) to config:"; \
		echo $(PARAM) > /proc/my_config ; \
		cat /proc/my_config ; \
	else \
		echo "To provide value to test execute 'make test-proc value'"; \
	fi
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5
	@echo ""
	@echo "Unloading module..."
	sudo rmmod proc_module
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5

test-sys-stats:
	@echo "=== Testing sys_stats_module ==="
	@echo "Loading module..."
	-@sudo rmmod sys_stats_module 2>/dev/null || true
	sudo insmod src/sys_stats_module.ko
	cat /proc/sys_stats
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5
	@echo ""
	@echo "Unloading module..."
	sudo rmmod sys_stats_module
	@echo ""
	@echo "Output:"
	@sudo dmesg | tail -5

%:
	@:
	
