import threading
import time
import sys
import argparse

class RingBuffer:
    def __init__(self, capacity, producers_total):
        self.data = [0] * capacity
        self.capacity = capacity
        self.head = 0
        self.tail = 0
        self.count = 0
        self.producers_active = producers_total
        self.mutex = threading.Lock()
        self.free_slots = threading.Semaphore(capacity)  
        self.used_slots = threading.Semaphore(0)       
        self.all_producers_done = threading.Event()

    def push(self, value):
        self.free_slots.acquire()  
        with self.mutex:
            self.data[self.tail] = value
            self.tail = (self.tail + 1) % self.capacity
            self.count += 1
        self.used_slots.release() 

    def pop(self):
        while True:
            acquired = self.used_slots.acquire(timeout=0.1)
        
            with self.mutex:
                if not acquired:
                    if self.count == 0 and self.all_producers_done.is_set():
                        return None
                    continue
            
                if self.count == 0:
                    self.used_slots.release()
                    if self.all_producers_done.is_set():
                        return None
                    continue
            
                value = self.data[self.head]
                self.head = (self.head + 1) % self.capacity
                self.count -= 1
                self.free_slots.release()
                return value

    def producer_done(self):
        with self.mutex:
            self.producers_active -= 1
            if self.producers_active == 0:
                self.all_producers_done.set()

class ProducerArgs:
    def __init__(self, rb, items_to_produce, producer_index):
        self.rb = rb
        self.items_to_produce = items_to_produce
        self.producer_index = producer_index

class ConsumerArgs:
    def __init__(self, rb, consumer_index):
        self.rb = rb
        self.consumed_sum = 0
        self.consumed_count = 0
        self.consumer_index = consumer_index

def producer_thread(args):
    for i in range(args.items_to_produce):
        value = (args.producer_index + 1) * 1000000 + i  
        args.rb.push(value)
    args.rb.producer_done()

def consumer_thread(args):
    while True:
        value = args.rb.pop()
        if value is None:
            break
        args.consumed_sum += value
        args.consumed_count += 1

def now_monotonic_ms():
    return int(time.monotonic() * 1000)

def main():
    parser = argparse.ArgumentParser(description="Демонстрация производителя-потребителя с threading.Semaphore")
    parser.add_argument("-P", "--producers", type=int, default=2, help="Количество производителей")
    parser.add_argument("-C", "--consumers", type=int, default=2, help="Количество потребителей")
    parser.add_argument("-N", "--items", type=int, default=100000, help="Общее количество элементов")
    parser.add_argument("-B", "--buffer-size", type=int, default=64, help="Размер буфера")
    args = parser.parse_args()

    if args.producers <= 0 or args.consumers <= 0 or args.buffer_size <= 0 or args.items < 0:
        print("Некорректные аргументы", file=sys.stderr)
        sys.exit(1)

    rb = RingBuffer(args.buffer_size, args.producers)
    producers = []
    consumers = []
    pargs = []
    cargs = []

    per_producer = args.items // args.producers
    remainder = args.items % args.producers

    start_ms = now_monotonic_ms()

    for i in range(args.producers):
        items = per_producer + (1 if i < remainder else 0)
        parg = ProducerArgs(rb, items, i)
        pargs.append(parg)
        t = threading.Thread(target=producer_thread, args=(parg,))
        producers.append(t)
        t.start()

    for i in range(args.consumers):
        carg = ConsumerArgs(rb, i)
        cargs.append(carg)
        t = threading.Thread(target=consumer_thread, args=(carg,))
        consumers.append(t)
        t.start()

    produced_total = 0
    for i in range(args.producers):
        producers[i].join()
        produced_total += pargs[i].items_to_produce

    for t in consumers:
        t.join()

    consumed_total = 0
    consumed_sum = 0
    for carg in cargs:
        consumed_total += carg.consumed_count
        consumed_sum += carg.consumed_sum

    end_ms = now_monotonic_ms()

    print(f"[prodcons] P={args.producers} C={args.consumers} N={args.items} B={args.buffer_size} "
          f"produced={produced_total} consumed={consumed_total} sum={consumed_sum} time_ms={end_ms - start_ms}")

if __name__ == "__main__":
    main()
