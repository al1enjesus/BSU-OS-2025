#!/usr/bin/env python3
import os
import mmap
import time
import sys

FILENAME = os.getenv("TESTFILE", "testfile.bin")
if len(sys.argv) > 1:
    FILENAME = sys.argv[1]

FILESIZE = 100 * 1024 * 1024

def create_test_file():
    if not os.path.exists(FILENAME):
        print(f"Создание файла {FILENAME} размером {FILESIZE // (1024 * 1024)} MB...")
        with open(FILENAME, "wb") as f:
            f.write(b"A" * FILESIZE)
        print("Файл создан.\n")

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
    total = 0
    with open(FILENAME, "rb") as f:
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
            total = sum(mm[i] for i in range(len(mm)))
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
    create_test_file()
    main()
