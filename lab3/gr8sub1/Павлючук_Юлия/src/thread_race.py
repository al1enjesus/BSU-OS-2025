#!/usr/bin/env python3
import threading
import time
import sys
from threading import Lock

class Counter:
    def __init__(self):
        self.value = 0

class UnsynchronizedCounter(Counter):
    def increment(self):
        self.value += 1

class MutexCounter(Counter):
    def __init__(self):
        super().__init__()
        self.lock = Lock()
    
    def increment(self):
        with self.lock:
            self.value += 1

class AtomicCounter(Counter):
    def __init__(self):
        super().__init__()
        self.lock = Lock()  # Имитация атомарности
    
    def increment(self):
        with self.lock:
            self.value += 1

def worker_thread(counter, increments_per_thread):
    for _ in range(increments_per_thread):
        counter.increment()

def run_experiment(num_threads, total_increments, mode):
    if mode == "unsync":
        counter = UnsynchronizedCounter()
    elif mode == "mutex":
        counter = MutexCounter()
    elif mode == "atomic":
        counter = AtomicCounter()
    else:
        raise ValueError(f"Unknown mode: {mode}")
    
    increments_per_thread = total_increments // num_threads
    
    threads = []
    start_time = time.time()
    
    for i in range(num_threads):
        thread = threading.Thread(target=worker_thread, args=(counter, increments_per_thread))
        threads.append(thread)
        thread.start()
    
    for thread in threads:
        thread.join()
    
    end_time = time.time()
    execution_time_ms = (end_time - start_time) * 1000
    
    print(f"mode={mode} threads={num_threads} expected={total_increments} actual={counter.value} time_ms={execution_time_ms:.2f}")
    
    return execution_time_ms, counter.value

def main():
    if len(sys.argv) != 4:
        print("Usage: python thread_race.py <N> <M> <mode>")
        print("  N - number of threads")
        print("  M - total increments") 
        print("  mode - unsync, mutex, or atomic")
        sys.exit(1)
    
    num_threads = int(sys.argv[1])
    total_increments = int(sys.argv[2])
    mode = sys.argv[3]
    
    run_experiment(num_threads, total_increments, mode)

if __name__ == "__main__":
    main()
