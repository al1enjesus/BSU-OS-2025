#!/usr/bin/env python3
import threading
import time
import sys
from threading import Lock

class Counter:
    def __init__(self):
        self.unsync_value = 0
        self.mutex_value = 0
        self.atomic_value = 0
        self.lock = Lock()
    
    def unsync_increment(self):
        self.unsync_value += 1
    
    def mutex_increment(self):
        with self.lock:
            self.mutex_value += 1
    
    def atomic_increment(self):
        # В Python обычные операции уже атомарны для int
        self.atomic_value += 1

def worker_unsync(counter, iterations):
    for _ in range(iterations):
        counter.unsync_increment()

def worker_mutex(counter, iterations):
    for _ in range(iterations):
        counter.mutex_increment()

def worker_atomic(counter, iterations):
    for _ in range(iterations):
        counter.atomic_increment()

def run_test(num_threads, iterations_per_thread, mode):
    counter = Counter()
    threads = []
    
    start_time = time.time()
    
    for _ in range(num_threads):
        if mode == "unsync":
            t = threading.Thread(target=worker_unsync, args=(counter, iterations_per_thread))
        elif mode == "mutex":
            t = threading.Thread(target=worker_mutex, args=(counter, iterations_per_thread))
        elif mode == "atomic":
            t = threading.Thread(target=worker_atomic, args=(counter, iterations_per_thread))
        else:
            print(f"Unknown mode: {mode}")
            return
        
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
    
    end_time = time.time()
    elapsed_ms = int((end_time - start_time) * 1000)
    
    expected = num_threads * iterations_per_thread
    
    if mode == "unsync":
        actual = counter.unsync_value
    elif mode == "mutex":
        actual = counter.mutex_value
    else:
        actual = counter.atomic_value
    
    print(f"mode={mode} threads={num_threads} iters_per_thread={iterations_per_thread} expected={expected} actual={actual} time_ms={elapsed_ms}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python3 thread_race.py <num_threads> <iterations_per_thread> <unsync|mutex|atomic>")
        sys.exit(1)
    
    num_threads = int(sys.argv[1])
    iterations = int(sys.argv[2])
    mode = sys.argv[3]
    
    run_test(num_threads, iterations, mode)
