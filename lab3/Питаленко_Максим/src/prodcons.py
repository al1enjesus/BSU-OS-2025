import threading
import time
import argparse
from threading import Semaphore
import sys

class BoundedBuffer:
    def __init__(self, size):
        self.buffer = [0] * size
        self.size = size
        self.head = 0
        self.tail = 0
        self.count = 0
        
        self.empty_slots = Semaphore(size)
        self.filled_slots = Semaphore(0)
        self.mutex = threading.Lock()
        
        self.shutdown = False
        
    def produce(self, item):
        self.empty_slots.acquire()
        with self.mutex:
            self.buffer[self.tail] = item
            self.tail = (self.tail + 1) % self.size
            self.count += 1
        self.filled_slots.release()
    
    def consume(self):
        self.filled_slots.acquire()
        with self.mutex:
            if self.shutdown and self.count == 0:
                self.filled_slots.release()
                return None
            item = self.buffer[self.head]
            self.head = (self.head + 1) % self.size
            self.count -= 1
        self.empty_slots.release()
        return item
    
    def stop(self):
        with self.mutex:
            self.shutdown = True
        for _ in range(10):
            self.filled_slots.release()

def producer(buffer, producer_id, items_count):
    for i in range(items_count):
        buffer.produce(producer_id * 1000 + i)
    print(f"Producer {producer_id} finished")

def consumer(buffer, consumer_id):
    count = 0
    while True:
        item = buffer.consume()
        if item is None:
            break
        count += 1
    print(f"Consumer {consumer_id} finished, consumed {count} items")

def main():
    parser = argparse.ArgumentParser(description='Producer-Consumer with semaphores')
    parser.add_argument('-P', '--producers', type=int, default=2, help='Number of producers')
    parser.add_argument('-C', '--consumers', type=int, default=2, help='Number of consumers')
    parser.add_argument('-N', '--items', type=int, default=1000, help='Items per producer')
    parser.add_argument('-B', '--buffer', type=int, default=10, help='Buffer size')
    
    args = parser.parse_args()
    
    print(f"Starting with {args.producers} producers, {args.consumers} consumers")
    print(f"{args.items} items per producer, buffer size: {args.buffer}")
    
    buffer = BoundedBuffer(args.buffer)
    
    consumers_list = []
    for i in range(args.consumers):
        c = threading.Thread(target=consumer, args=(buffer, i))
        consumers_list.append(c)
        c.start()
    
    producers_list = []
    for i in range(args.producers):
        p = threading.Thread(target=producer, args=(buffer, i, args.items))
        producers_list.append(p)
        p.start()
    
    for p in producers_list:
        p.join()

    buffer.stop()
    
    for c in consumers_list:
        c.join()
    
    print("All threads finished")

if __name__ == "__main__":
    main()