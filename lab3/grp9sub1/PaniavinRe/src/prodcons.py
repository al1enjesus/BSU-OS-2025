#!/usr/bin/env python3
import threading
import queue
import time
import sys
import argparse

class RingBuffer:
    def __init__(self, capacity):
        self.capacity = capacity
        self.buffer = [None] * capacity
        self.head = 0
        self.tail = 0
        self.count = 0
        self.lock = threading.Lock()
        self.not_empty = threading.Condition(self.lock)
        self.not_full = threading.Condition(self.lock)
        self.producers_done = False
    
    def push(self, value):
        with self.not_full:
            while self.count == self.capacity:
                self.not_full.wait()
            self.buffer[self.tail] = value
            self.tail = (self.tail + 1) % self.capacity
            self.count += 1
            self.not_empty.notify()
    
    def pop(self):
        with self.not_empty:
            while self.count == 0 and not self.producers_done:
                self.not_empty.wait()
            if self.count == 0 and self.producers_done:
                return None
            value = self.buffer[self.head]
            self.head = (self.head + 1) % self.capacity
            self.count -= 1
            self.not_full.notify()
            return value
    
    def set_producers_done(self):
        with self.not_empty:
            self.producers_done = True
            self.not_empty.notify_all()

def producer_thread(rb, items_to_produce, producer_index, results):
    produced_count = 0
    for i in range(items_to_produce):
        value = (producer_index + 1) * 1000000 + i
        rb.push(value)
        produced_count += 1
    results[producer_index] = produced_count

def consumer_thread(rb, consumer_index, results):
    consumed_sum = 0
    consumed_count = 0
    while True:
        value = rb.pop()
        if value is None:
            break
        consumed_sum += value
        consumed_count += 1
    results[consumer_index] = (consumed_count, consumed_sum)

def main():
    parser = argparse.ArgumentParser(description='Producer-Consumer with bounded buffer')
    parser.add_argument('-P', '--producers', type=int, default=2, help='Number of producers')
    parser.add_argument('-C', '--consumers', type=int, default=2, help='Number of consumers')
    parser.add_argument('-N', '--items', type=int, default=100000, help='Total number of items')
    parser.add_argument('-B', '--buffer', type=int, default=64, help='Buffer size')
    
    args = parser.parse_args()
    
    P = args.producers
    C = args.consumers
    N = args.items
    B = args.buffer
    
    rb = RingBuffer(B)
    
    # Запускаем производителей
    producer_results = [0] * P
    producer_threads = []
    
    items_per_producer = N // P
    remainder = N % P
    
    for i in range(P):
        items = items_per_producer + (1 if i < remainder else 0)
        t = threading.Thread(target=producer_thread, args=(rb, items, i, producer_results))
        producer_threads.append(t)
        t.start()
    
    # Запускаем потребителей
    consumer_results = [None] * C
    consumer_threads = []
    
    for i in range(C):
        t = threading.Thread(target=consumer_thread, args=(rb, i, consumer_results))
        consumer_threads.append(t)
        t.start()
    
    # Ждем завершения производителей
    for t in producer_threads:
        t.join()
    
    # Сигнализируем потребителям о завершении
    rb.set_producers_done()
    
    # Ждем завершения потребителей
    for t in consumer_threads:
        t.join()
    
    # Считаем результаты
    produced_total = sum(producer_results)
    consumed_total = 0
    consumed_sum = 0
    
    for count, sum_val in consumer_results:
        consumed_total += count
        consumed_sum += sum_val
    
    print(f"[prodcons] P={P} C={C} N={N} B={B} produced={produced_total} consumed={consumed_total} sum={consumed_sum}")
    
    # Проверка корректности
    if produced_total != N:
        print(f"[ERROR] Produced {produced_total} != N {N}")
    if consumed_total != N:
        print(f"[ERROR] Consumed {consumed_total} != N {N}")
    
    # Проверка суммы
    expected_sum = 0
    for i in range(P):
        k = producer_results[i]
        base = (i + 1) * 1000000
        expected_sum += k * base + (k * (k - 1)) // 2
    
    if consumed_sum == expected_sum:
        print(f"[OK] Sum check passed: {consumed_sum}")
    else:
        print(f"[WARNING] Sum mismatch: got {consumed_sum}, expected {expected_sum}")

if __name__ == "__main__":
    main()
