#!/usr/bin/env python3
import os
import mmap

def read_file(path, filter_prefix=None):
    """Читает файл построчно, можно фильтровать по префиксу"""
    try:
        with open(path, "r") as f:
            lines = f.readlines()
            if filter_prefix:
                lines = [l for l in lines if l.startswith(filter_prefix)]
            return "".join(lines)
    except FileNotFoundError:
        return f"[Файл {path} не найден]\n"

def print_memory_info(stage):
    pid = os.getpid()
    print(f"\n=== {stage} ===")
    print(f"PID: {pid}")

    # /proc/self/maps
    print("\n--- /proc/self/maps ---")
    print(read_file("/proc/self/maps"))

    # /proc/self/status (VmSize, VmRSS)
    print("\n--- /proc/self/status (Vm*) ---")
    print(read_file("/proc/self/status", "Vm"))

    # /proc/self/smaps_rollup (Pss, Private_*)
    print("\n--- /proc/self/smaps_rollup ---")
    print(read_file("/proc/self/smaps_rollup"))

def main():
    print_memory_info("До выделения памяти")

    stack_var = bytearray(1024)
    heap_var = bytearray(1024 * 1024)

    mmap_var = mmap.mmap(-1, 1024 * 1024, access=mmap.ACCESS_WRITE)
    heap_var[0:4] = b"HEAP"
    mmap_var[0:4] = b"MMAP"

    print_memory_info("После выделения памяти")

    input("\nНажмите Enter для выхода...")

    mmap_var.close()

if __name__ == "__main__":
    main()
