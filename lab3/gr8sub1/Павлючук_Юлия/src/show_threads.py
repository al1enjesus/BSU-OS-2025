#!/usr/bin/env python3
import threading
import time
import os
import subprocess

def worker(worker_id):
    """Рабочая функция потока"""
    print(f"Worker {worker_id} started")
    for i in range(10):
        time.sleep(0.5)
    print(f"Worker {worker_id} finished")

def show_threads_info():
    """Показывает информацию о потоках текущего процесса"""
    pid = os.getpid()
    
    print(f"\n{'='*60}")
    print(f"ИНФОРМАЦИЯ О ПОТОКАХ ДЛЯ PID: {pid}")
    print(f"{'='*60}")
    
    # 1. ps -L -p <PID>
    print(f"\n1. ps -L -p {pid} -o pid,tid,psr,pcpu,stat,comm")
    print("-" * 60)
    try:
        result = subprocess.run(
            ['ps', '-L', '-p', str(pid), '-o', 'pid,tid,psr,pcpu,stat,comm'],
            capture_output=True, text=True
        )
        # Берем первые 20 строк
        lines = result.stdout.strip().split('\n')
        for line in lines[:20]:
            print(line)
    except Exception as e:
        print(f"Ошибка: {e}")
    
    # 2. /proc/<PID>/status | grep Threads
    print(f"\n2. cat /proc/{pid}/status | grep Threads")
    print("-" * 60)
    try:
        with open(f'/proc/{pid}/status', 'r') as f:
            for line in f:
                if 'Threads' in line:
                    print(line.strip())
    except Exception as e:
        print(f"Ошибка чтения /proc/status: {e}")
    
    # 3. ls -l /proc/<PID>/task
    print(f"\n3. ls -l /proc/{pid}/task")
    print("-" * 60)
    try:
        result = subprocess.run(
            ['ls', '-l', f'/proc/{pid}/task'],
            capture_output=True, text=True
        )
        # Берем первые 10 строк
        lines = result.stdout.strip().split('\n')
        for line in lines[:10]:
            print(line)
    except Exception as e:
        print(f"Ошибка чтения /proc/task: {e}")
    
    # 4. Дополнительная информация
    print(f"\n4. Дополнительная информация")
    print("-" * 60)
    print(f"Активных потоков в Python: {threading.active_count()}")
    print(f"Текущий поток: {threading.current_thread().name}")

def main():
    print("Создаем 4 рабочих потока...")
    
    # Создаем потоки
    threads = []
    for i in range(4):
        thread = threading.Thread(target=worker, args=(i,), name=f"Worker-{i}")
        threads.append(thread)
        thread.start()
    
    # Даем время потокам запуститься
    time.sleep(2)
    
    # Показываем информацию
    show_threads_info()
    
    # Ждем завершения
    print(f"\nОжидаем завершения потоков...")
    for thread in threads:
        thread.join()
    
    print("Все потоки завершены!")

if __name__ == "__main__":
    main()
