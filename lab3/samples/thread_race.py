import threading
import time
import sys
import argparse
import ctypes
import os


class AtomicCounterCTypes:
    """Атомарный счетчик с использованием ctypes и собственной C-библиотеки libatomic.so"""
    def __init__(self, initial=0):
        self._value = ctypes.c_int(initial)
        lib_path = os.path.join(os.path.dirname(__file__), "libatomic.so")
        self._lib = ctypes.CDLL(lib_path)
        self._lib.atomic_increment.argtypes = [ctypes.POINTER(ctypes.c_int)]
        self._lib.atomic_increment.restype = ctypes.c_int
    
    def atomic_increment(self, num=1):
        for _ in range(num):
            self._lib.atomic_increment(ctypes.byref(self._value))
    
    def get_value(self):
        return self._value.value

def now_monotonic_ms():
    return int(time.monotonic() * 1000)

def worker_unsync(args, counter):
    for _ in range(args.iters):
        counter[0] += 1
        
def worker_mutex(args, counter, lock):
    for _ in range(args.iters):
        with lock:
            counter[0] += 1

def worker_atomic(args, counter, _):
    for _ in range(args.iters):
        counter.atomic_increment()

def main():
    parser = argparse.ArgumentParser(description="Демонстрация гонки данных в потоках")
    parser.add_argument("threads", type=int, help="Количество потоков")
    parser.add_argument("iters", type=int, help="Количество итераций на поток")
    parser.add_argument("mode", choices=["unsync", "mutex", "atomic"], help="Режим: unsync, mutex или atomic")
    args = parser.parse_args()

    if args.threads <= 0 or args.iters < 0:
        print("Некорректные аргументы", file=sys.stderr)
        sys.exit(1)

    if args.mode == "atomic":
        counter = AtomicCounterCTypes(0)
    else:
        counter = [0]
    lock = threading.Lock()
    threads = []

    print(f"[thread_race] Запуск с {args.threads} потоками, {args.iters} итерациями, режим={args.mode}", file=sys.stderr)

    start_ms = now_monotonic_ms()

    for i in range(args.threads):
        if args.mode == "unsync":
            t = threading.Thread(target=worker_unsync, args=(args, counter))
        elif args.mode == "mutex":
            t = threading.Thread(target=worker_mutex, args=(args, counter, lock))
        else:
            t = threading.Thread(target=worker_atomic, args=(args, counter, lock))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    end_ms = now_monotonic_ms()
    expected = args.threads * args.iters
    actual = counter.get_value() if args.mode == "atomic" else counter[0]


    print(f"mode={args.mode} threads={args.threads} iters_per_thread={args.iters} "
          f"expected={expected} actual={actual} time_ms={end_ms - start_ms}")

if __name__ == "__main__":
    main()
