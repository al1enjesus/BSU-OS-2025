#!/usr/bin/env python3
import os
import time
import statistics

def measure_time(func):
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        end = time.time()
        return result, end - start
    return wrapper

@measure_time
def write_stdio(filename, data_size=100*1024*1024):
    with open(filename, 'wb') as f:
        for i in range(data_size):
            f.write(b'A')
    return data_size

@measure_time
def write_syscall_buffered(filename, data_size=100*1024*1024, buffer_size=4096):
    fd = os.open(filename, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
    chunk = b'A' * buffer_size
    total_written = 0
    
    try:
        while total_written < data_size:
            if total_written + buffer_size > data_size:
                write_size = data_size - total_written
                chunk = b'A' * write_size
            else:
                write_size = buffer_size
                
            os.write(fd, chunk)
            total_written += write_size
    finally:
        os.close(fd)
    
    return total_written

def run_benchmarks():
    data_size = 100 * 1024 * 1024
    test_files = []
    
    print("=== СРАВНЕНИЕ I/O МЕТОДОВ ===")
    print(f"Размер данных: {data_size // (1024*1024)} MB")
    print()
    
    results = []
    
    print("1. Стандартная библиотека (буферизованный):")
    _, time_stdio = write_stdio("test_stdio.bin", data_size)
    results.append(("stdio_buffered", time_stdio))
    print(f"   Время: {time_stdio:.2f} сек")
    test_files.append("test_stdio.bin")
    
    print("\n2. Системные вызовы с разными размерами буфера:")
    
    buffer_sizes = [512, 1024, 4096, 8192, 16384, 32768, 65536]
    
    for buf_size in buffer_sizes:
        filename = f"test_syscall_{buf_size}.bin"
        _, time_syscall = write_syscall_buffered(filename, data_size, buf_size)
        results.append((f"syscall_{buf_size}", time_syscall))
        test_files.append(filename)
        speed = (data_size / (1024*1024)) / time_syscall
        syscalls = data_size / buf_size
        print(f"   Буфер {buf_size:5} байт: {time_syscall:.2f} сек, {speed:.1f} MB/сек, ~{int(syscalls)} syscall")
    
    print("\n=== РЕЗУЛЬТАТЫ ===")
    print("Метод                Время(сек)   Скорость(MB/сек)")
    print("-" * 50)
    
    for method, t in results:
        speed = (data_size / (1024*1024)) / t
        if "syscall" in method:
            buf_size = method.split("_")[1]
            print(f"syscall {buf_size:>8}    {t:7.2f}      {speed:8.1f}")
        else:
            print(f"{method:20} {t:7.2f}      {speed:8.1f}")
    
    best_method = min(results, key=lambda x: x[1])
    worst_method = max(results, key=lambda x: x[1])
    
    print(f"\nЛучший результат: {best_method[0]} - {best_method[1]:.2f} сек")
    print(f"Худший результат: {worst_method[0]} - {worst_method[1]:.2f} сек")
    
    for f in test_files:
        try:
            os.remove(f)
        except:
            pass

if __name__ == "__main__":
    run_benchmarks()
