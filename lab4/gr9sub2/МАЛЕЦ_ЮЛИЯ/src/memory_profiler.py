#!/usr/bin/env python3
import os
import mmap
import time

FILENAME = "testfile.bin"
FILESIZE = 100 * 1024 * 1024

def read_with_syscalls():
    """Чтение файла через open() + read()"""
    total = 0
    with open(FILENAME, "rb") as f:
        while True:
            data = f.read(1024 * 1024)
            if not data:
                break
            total += sum(data)
    return total

def read_with_mmap():
    """Чтение файла через mmap()"""
    with open(FILENAME, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        total = sum(mm[i] for i in range(len(mm)))
        mm.close()
    return total


def benchmark(func, name):
    start = time.time()
    result = func()
    end = time.time()
    print(f"{name}: сумма байтов={result}, время={end - start:.4f} сек")

def main():
    print("PID процесса:", os.getpid())
    input("Нажмите Enter для продолжения...")
    print("=== Тест производительности ===")
    benchmark(read_with_syscalls, "read()/write()")
    benchmark(read_with_mmap, "mmap()")

if __name__ == "__main__":
    main()
