#!/usr/bin/env python3
import os
import time
import subprocess
import threading
import sys

class DiskIOMonitor:
    def __init__(self):
        self.stop_monitoring = False
        self.test_files = []
        
    def create_io_workload(self, duration=30):
        print("=== СОЗДАНИЕ DISK I/O НАГРУЗКИ ===")
        
        def write_workload():
            start_time = time.time()
            file_count = 0
            
            while time.time() - start_time < duration and not self.stop_monitoring:
                filename = f"test_io_{file_count}.dat"
                try:
                    with open(filename, 'wb') as f:
                        data = os.urandom(1024 * 1024)
                        for _ in range(10):
                            f.write(data)
                    self.test_files.append(filename)
                    file_count += 1
                except Exception as e:
                    print(f"Ошибка записи: {e}")
                    
        def read_workload():
            start_time = time.time()
            
            while time.time() - start_time < duration and not self.stop_monitoring:
                if self.test_files:
                    for filename in self.test_files[-5:]:
                        try:
                            with open(filename, 'rb') as f:
                                while f.read(1024 * 64):
                                    if self.stop_monitoring:
                                        return
                        except:
                            pass
                time.sleep(0.1)
        
        write_thread = threading.Thread(target=write_workload)
        read_thread = threading.Thread(target=read_workload)
        
        write_thread.start()
        read_thread.start()
        
        return write_thread, read_thread

    def run_iostat(self):
        print("\n=== iostat -x 1 10 ===")
        try:
            result = subprocess.run(['iostat', '-x', '1', '10'], 
                                  capture_output=True, text=True, timeout=15)
            print(result.stdout)
        except Exception as e:
            print(f"Ошибка iostat: {e}")

    def run_iotop(self):
        print("\n=== iotop -o 1 10 ===")
        try:
            result = subprocess.run(['timeout', '12s', 'iotop', '-o', '1', '10'], 
                                  capture_output=True, text=True)
            print(result.stdout)
        except Exception as e:
            print(f"Ошибка iotop: {e}")

    def check_io_scheduler(self):
        print("\n=== I/O SCHEDULER ===")
        try:
            disks = ['sda', 'sdb', 'nvme0n1', 'vda']
            for disk in disks:
                path = f"/sys/block/{disk}/queue/scheduler"
                if os.path.exists(path):
                    with open(path, 'r') as f:
                        scheduler = f.read().strip()
                    print(f"Диск {disk}: {scheduler}")
        except Exception as e:
            print(f"Ошибка проверки scheduler: {e}")

    def monitor_proc_io(self, pid=None):
        if pid is None:
            pid = os.getpid()
            
        print(f"\n=== /proc/{pid}/io ===")
        try:
            with open(f'/proc/{pid}/io', 'r') as f:
                io_stats = f.read()
            print(io_stats)
        except Exception as e:
            print(f"Ошибка чтения /proc/io: {e}")

    def run_pidstat(self):
        print("\n=== pidstat -d 1 10 ===")
        try:
            result = subprocess.run(['pidstat', '-d', '1', '10'], 
                                  capture_output=True, text=True, timeout=15)
            print(result.stdout)
        except Exception as e:
            print(f"Ошибка pidstat: {e}")

    def cleanup(self):
        print("\n=== ОЧИСТКА ===")
        for filename in self.test_files:
            try:
                os.remove(filename)
            except:
                pass
        print(f"Удалено {len(self.test_files)} временных файлов")

    def run_benchmark(self):
        print("ЗАПУСК ДИСКОВОГО БЕНЧМАРКА...")
        print("Нагрузка: чтение/запись в течение 30 секунд")
        
        self.check_io_scheduler()
        
        workload_threads = self.create_io_workload(30)
        
        time.sleep(2)
        
        monitor_thread = threading.Thread(target=self.run_iostat)
        monitor_thread.start()
        
        time.sleep(5)
        
        self.run_pidstat()
        
        time.sleep(5)
        
        self.monitor_proc_io()
        
        print("\nОжидание завершения нагрузки...")
        self.stop_monitoring = True
        
        for thread in workload_threads:
            thread.join(timeout=5)
        
        monitor_thread.join(timeout=5)
        
        self.cleanup()

if __name__ == "__main__":
    monitor = DiskIOMonitor()
    
    try:
        monitor.run_benchmark()
        analyze_disk_performance()
    except KeyboardInterrupt:
        print("\nПрервано пользователем")
        monitor.stop_monitoring = True
        monitor.cleanup()
    except Exception as e:
        print(f"Ошибка: {e}")
        monitor.cleanup()
