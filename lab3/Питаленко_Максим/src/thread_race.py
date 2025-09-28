import threading
import time
import argparse
from threading import Lock

class Counter:
    def __init__(self):
        self.value = 0
        self.lock = Lock()
    
    def increment_unsync(self):
        temp = self.value
        self.value = temp + 1
    
    def increment_mutex(self):
        with self.lock:
            temp = self.value
            self.value = temp + 1
    
    def increment_atomic(self):
        with self.lock:
            self.value += 1

def worker_unsync(counter, iterations):
    for _ in range(iterations):
        counter.increment_unsync()

def worker_mutex(counter, iterations):
    for _ in range(iterations):
        counter.increment_mutex()

def worker_atomic(counter, iterations):
    for _ in range(iterations):
        counter.increment_atomic()

def main():
    parser = argparse.ArgumentParser(description='Thread race condition experiment')
    parser.add_argument('threads', type=int, help='Number of threads')
    parser.add_argument('iterations', type=int, help='Iterations per thread')
    parser.add_argument('mode', choices=['unsync', 'mutex', 'atomic'], help='Synchronization mode')
    
    args = parser.parse_args()
    
    counter = Counter()
    threads = []
    
    start_time = time.time()
    
    if args.mode == 'unsync':
        worker_func = worker_unsync
    elif args.mode == 'mutex':
        worker_func = worker_mutex
    else: 
        worker_func = worker_atomic
    
    for _ in range(args.threads):
        thread = threading.Thread(target=worker_func, args=(counter, args.iterations))
        threads.append(thread)
        thread.start()
    
    for thread in threads:
        thread.join()
    
    end_time = time.time()
    
    expected = args.threads * args.iterations
    actual = counter.value
    
    print(f"Mode: {args.mode}")
    print(f"Threads: {args.threads}, Iterations per thread: {args.iterations}")
    print(f"Expected: {expected}, Actual: {actual}")
    print(f"Time: {end_time - start_time:.4f} seconds")
    print(f"Correct: {expected == actual}")

if __name__ == "__main__":
    main()