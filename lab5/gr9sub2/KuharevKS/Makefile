 # Makefile для Lab 5 - Модули ядра Linux (Arch Linux)

PWD := $(shell pwd)
KERNEL_DIR := /usr/lib/modules/$(shell uname -r)/build

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m :=

all: hello_module proc_module chardev_module
  @echo "✓ All modules built successfully!"

hello_module:
  @echo "Building hello_module..."
  @echo "obj-m += hello_module.o" > $(PWD)/hello_module.tmp
  $(MAKE) -C $(KERNEL_DIR) M=$(PWD) EXTRA_CFLAGS="$(ccflags-y)" obj-m="hello_module.o" modules
  @rm -f $(PWD)/hello_module.tmp
  @echo "✓ hello_module built!"

proc_module:
  @echo "Building proc_module..."
  @echo "obj-m += proc_module.o" > $(PWD)/proc_module.tmp
  $(MAKE) -C $(KERNEL_DIR) M=$(PWD) EXTRA_CFLAGS="$(ccflags-y)" obj-m="proc_module.o" modules
  @rm -f $(PWD)/proc_module.tmp
  @echo "✓ proc_module built!"

chardev_module:
  @echo "Building chardev_module..."
  @echo "obj-m += chardev_module.o" > $(PWD)/chardev_module.tmp
  $(MAKE) -C $(KERNEL_DIR) M=$(PWD) EXTRA_CFLAGS="$(ccflags-y)" obj-m="chardev_module.o" modules
  @rm -f $(PWD)/chardev_module.tmp
  @echo "✓ chardev_module built!"

clean:
  @echo "Cleaning up kernel modules..."
  $(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
  rm -f *.o *.ko *.mod.c *.mod *.symvers *.order .*.cmd *.tmp
  rm -rf .tmp_versions
  @echo "✓ Cleanup complete"
load:
  @if [ -z "$(MODULE)" ]; then \
    echo "Error: Please specify MODULE=<module_name>"; \
    exit 1; \
  fi
  @if [ ! -f $(MODULE).ko ]; then \
    echo "Error: $(MODULE).ko not found. Run 'make' first."; \
    exit 1; \
  fi
  @echo "Loading $(MODULE).ko..."
  sudo insmod $(MODULE).ko
  @echo "✓ Module loaded. Check dmesg | tail or journalctl -k -f for output"

unload:
  @if [ -z "$(MODULE)" ]; then \
    echo "Error: Please specify MODULE=<module_name>"; \
    exit 1; \
  fi
  @echo "Unloading $(MODULE)..."
  sudo rmmod $(MODULE)
  @echo "✓ Module unloaded. Check dmesg | tail for output"

info:
  @echo "=== Loaded Modules ==="
  @lsmod | head -1
  @lsmod | grep -E "(hello_module|proc_module|chardev_module)" || echo "No lab5 modules loaded"
  @echo ""
  @echo "=== Built Modules ==="
  @ls -lh *.ko 2>/dev/null || echo "No modules built yet (run 'make')"

test-hello: all
  @echo "=== Testing hello_module ==="
  -@sudo rmmod hello_module 2>/dev/null || true
  sudo insmod hello_module.ko
  @dmesg | tail -5
  sudo rmmod hello_module
  @dmesg | tail -5

test-proc: all
  @echo "=== Testing proc_module ==="
  -@sudo rmmod proc_module 2>/dev/null || true
  sudo insmod proc_module.ko
  @cat /proc/student_info 2>/dev/null || echo "File not created (check dmesg)"
  sudo rmmod proc_module
  @dmesg | tail -5

test-chardev: all
  @echo "=== Testing chardev_module ==="
  -@sudo rmmod chardev_module 2>/dev/null || true
  -@sudo rm /dev/mychardev 2>/dev/null || true
  sudo insmod chardev_module.ko
  @MAJOR=$$(dmesg | grep "chardev: Registered" | tail -1 | grep -o "number [0-9]*" | awk '{print $$2}'); \
  if [ -n "$$MAJOR" ]; then \
    sudo mknod /dev/mychardev c $$MAJOR 0; \
    sudo chmod 666 /dev/mychardev; \
    echo "Test data" > /dev/mychardev; \
    cat /dev/mychardev; \
    sudo rm /dev/mychardev; \
  else \
    echo "Failed to get major number (check dmesg)"; \
  fi
  sudo rmmod chardev_module
  @dmesg | tail -10

check:
  @echo "=== Environment Check ==="
  @echo -n "Kernel version: "
  @uname -r
  @echo -n "Kernel headers: "
  @if [ -d "$(KERNEL_DIR)" ]; then \
    echo "✓ Found at $(KERNEL_DIR)"; \
  else \
    echo "✗ NOT FOUND"; \
    echo "  Install with: sudo pacman -S linux-headers"; \
  fi
  @echo -n "Build tools: "
  @which gcc > /dev/null && echo "✓ gcc installed" || echo "✗ gcc NOT installed"
  @echo -n "Make: "
  @which make > /dev/null && echo "✓ make installed" || echo "✗ make NOT installed"
  @echo -n "Module loading: "
  @which insmod > /dev/null && echo "✓ insmod available" || echo "✗ insmod NOT available"
  @echo ""

install-deps:
  @echo "=== Installing dependencies ==="
  @sudo pacman -S --needed linux-headers base-devel


help:
  @echo "Lab 5 Kernel Modules - Makefile"
  @echo ""
  @echo "Available targets:"
  @echo "  make                     - Build all modules"
  @echo "  make clean               - Remove built modules"
  @echo "  make check               - Check environment"
  @echo "  make info                - Show built/loaded modules"
  @echo "  make load MODULE=xxx     - Load module"
  @echo "  make unload MODULE=xxx   - Unload module"
  @echo "  make test-hello          - Quick hello_module test"
  @echo "  make test-proc           - Quick proc_module test"
  @echo "  make test-chardev        - Quick chardev_module test"
  @echo "  make install-deps        - Install required packages"
  @echo ""
