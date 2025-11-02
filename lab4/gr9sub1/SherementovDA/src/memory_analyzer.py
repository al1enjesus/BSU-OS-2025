#!/usr/bin/env python3

import os
import sys
import mmap
import ctypes
import subprocess
import time

class MemoryAnalyzer:
    def __init__(self):
        self.heap_var = None
        self.mmap_var = None
        self.stack_var = None
        self.pid = os.getpid()
        
    def allocate_memory(self):
        print("=== ВЫДЕЛЕНИЕ ПАМЯТИ ===")
        
        self.stack_var = bytearray(1024)
        print("✓ Выделено 1 KB в стеке")
        
        self.heap_var = bytearray(1024 * 1024)
        print("✓ Выделено 1 MB в куче")
        
        try:
            self.mmap_var = mmap.mmap(-1, 1024 * 1024, 
                                    access=mmap.ACCESS_WRITE,
                                    flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
            print("✓ Выделено 1 MB через mmap")
        except Exception as e:
            print(f"✗ Ошибка mmap: {e}")
            print("Продолжаем без mmap...")
            self.mmap_var = None
        
        try:
            libc = ctypes.CDLL("libc.so.6")
            self.c_heap_var = libc.malloc(1024 * 1024)
            if self.c_heap_var:
                print("✓ Выделено 1 MB через malloc (C)")
            else:
                print("✗ Ошибка malloc")
        except Exception as e:
            print(f"✗ Ошибка malloc: {e}")
            self.c_heap_var = None

    def print_memory_map(self):
        print("\n=== КАРТА ПАМЯТИ /proc/self/maps ===")
        try:
            with open('/proc/self/maps', 'r') as f:
                maps = f.readlines()
                for line in maps:
                    if 'heap' in line or 'stack' in line or 'anon' in line:
                        print(line.strip())
        except Exception as e:
            print(f"Ошибка чтения maps: {e}")

    def get_memory_metrics(self):
        metrics = {}
        
        try:
            with open('/proc/self/status', 'r') as f:
                for line in f:
                    if line.startswith('VmSize:'):
                        metrics['vsz'] = int(line.split()[1])
                    elif line.startswith('VmRSS:'):
                        metrics['rss'] = int(line.split()[1])
        except Exception as e:
            print(f"Ошибка чтения status: {e}")

        return metrics

    def print_metrics(self, metrics, title):
        print(f"\n=== {title} ===")
        print(f"VSZ (VmSize): {metrics.get('vsz', 0):,} KB")
        print(f"RSS (VmRSS):  {metrics.get('rss', 0):,} KB")

    def run_system_commands(self):
        print("\n=== СИСТЕМНЫЕ УТИЛИТЫ ===")
        
        try:
            cmd = f"ps -o pid,comm,vsz,rss -p {self.pid}"
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
            print(result.stdout)
        except Exception as e:
            print(f"Ошибка выполнения команды: {e}")

def main():
    analyzer = MemoryAnalyzer()
    
    print("ДО выделения памяти:")
    metrics_before = analyzer.get_memory_metrics()
    analyzer.print_metrics(metrics_before, "МЕТРИКИ ДО")
    
    analyzer.allocate_memory()
    time.sleep(1)
    
    print("\nПОСЛЕ выделения памяти:")
    metrics_after = analyzer.get_memory_metrics()
    analyzer.print_metrics(metrics_after, "МЕТРИКИ ПОСЛЕ")
    
    analyzer.print_memory_map()
    analyzer.run_system_commands()
    
if __name__ == "__main__":
    main()
