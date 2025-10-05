#!/usr/bin/env python3
import threading
import time
import argparse
import random

class BoundedBuffer:
    def __init__(self, capacity):
        self.capacity = capacity
        self.buffer = [None] * capacity
        self.size = 0
        self.front = 0
        self.rear = 0
        
        # Синхронизация: mutex + condition variables
        self.mutex = threading.Lock()
        self.not_full = threading.Condition(self.mutex)   # Сигнал: есть свободные места
        self.not_empty = threading.Condition(self.mutex)  # Сигнал: есть элементы
        
        self.producer_count = 0
        self.consumer_count = 0
        self.shutdown = False  # Флаг корректного завершения

    def produce(self, item):
        """Добавление элемента в буфер с синхронизацией"""
        with self.mutex:
            # Ожидаем, пока не появится свободное место ИЛИ не поступит сигнал завершения
            while self.size == self.capacity and not self.shutdown:
                self.not_full.wait()  # Блокируемся без busy-wait
            
            if self.shutdown:
                return False  # Завершаем работу
            
            # Критическая секция - запись в буфер
            self.buffer[self.rear] = item
            self.rear = (self.rear + 1) % self.capacity
            self.size += 1
            self.producer_count += 1
            
            # Сигнализируем потребителям, что появились элементы
            self.not_empty.notify()
            return True

    def consume(self):
        """Извлечение элемента из буфера с синхронизацией"""
        with self.mutex:
            # Ожидаем, пока не появятся элементы ИЛИ не поступит сигнал завершения
            while self.size == 0 and not self.shutdown:
                self.not_empty.wait()  # Блокируемся без busy-wait
            
            # Если завершение и буфер пуст - выходим
            if self.shutdown and self.size == 0:
                return None
            
            # Критическая секция - чтение из буфера
            item = self.buffer[self.front]
            self.front = (self.front + 1) % self.capacity
            self.size -= 1
            self.consumer_count += 1
            
            # Сигнализируем производителям, что появилось свободное место
            self.not_full.notify()
            return item

    def stop(self):
        """Корректное завершение работы (без зависаний)"""
        with self.mutex:
            self.shutdown = True
            # Broadcast для разблокировки ВСЕХ ожидающих потоков
            self.not_full.notify_all()
            self.not_empty.notify_all()

def producer(buffer, producer_id, items_to_produce):
    """Функция производителя - генерирует элементы"""
    try:
        for i in range(items_to_produce):
            # Формула из задания: value = (producer_index + 1) * 10^6 + i
            item = (producer_id + 1) * 1000000 + i
            
            if not buffer.produce(item):
                break  # Завершаем если получили сигнал остановки
            
            # Имитация работы (10% chance)
            if random.random() < 0.1:
                time.sleep(0.001)
                
    except Exception as e:
        print(f"Producer {producer_id} error: {e}")

def consumer(buffer, consumer_id, consumed_items):
    """Функция потребителя - извлекает элементы"""
    try:
        while True:
            item = buffer.consume()
            if item is None:  # Сигнал завершения
                break
            consumed_items.append(item)
            
            # Имитация работы (10% chance)
            if random.random() < 0.1:
                time.sleep(0.001)
                
    except Exception as e:
        print(f"Consumer {consumer_id} error: {e}")

def calculate_expected_sum(producers, items_per_producer):
    """Вычисляет ожидаемую сумму по формуле из задания"""
    expected_sum = 0
    for i in range(producers):
        k_i = items_per_producer  # сколько элементов выпустил i-й продюсер
        # Формула: S = Σ[k_i * (i+1) * 10^6 + k_i*(k_i-1)/2]
        term1 = k_i * (i + 1) * 1000000
        term2 = (k_i * (k_i - 1)) // 2
        expected_sum += term1 + term2
    return expected_sum

def main():
    parser = argparse.ArgumentParser(description='Producer-Consumer with mutex+condition variables')
    parser.add_argument('-P', '--producers', type=int, default=2, help='Number of producers')
    parser.add_argument('-C', '--consumers', type=int, default=2, help='Number of consumers')
    parser.add_argument('-N', '--items', type=int, default=1000, help='Items per producer')
    parser.add_argument('-B', '--buffer', type=int, default=10, help='Buffer capacity')
    
    args = parser.parse_args()
    
    print(f"=== Producer-Consumer (mutex+condvar) ===")
    print(f"Producers: {args.producers}")
    print(f"Consumers: {args.consumers}")
    print(f"Items per producer: {args.items}")
    print(f"Buffer capacity: {args.buffer}")
    print(f"Total items: {args.producers * args.items}")
    
    # Создаем буфер и структуры для отслеживания
    buffer = BoundedBuffer(args.buffer)
    all_consumed_items = [[] for _ in range(args.consumers)]
    
    # Запускаем потребителей ПЕРВЫМИ
    consumer_threads = []
    for i in range(args.consumers):
        thread = threading.Thread(
            target=consumer, 
            args=(buffer, i, all_consumed_items[i])
        )
        consumer_threads.append(thread)
        thread.daemon = True
        thread.start()
    
    # Даем потребителям время для запуска
    time.sleep(0.1)
    
    # Запускаем производителей
    producer_threads = []
    start_time = time.time()
    
    for i in range(args.producers):
        thread = threading.Thread(
            target=producer,
            args=(buffer, i, args.items)
        )
        producer_threads.append(thread)
        thread.start()
    
    # Ждем завершения производителей
    for thread in producer_threads:
        thread.join()
    
    # Корректное завершение потребителей
    buffer.stop()
    
    # Ждем завершения потребителей
    for thread in consumer_threads:
        thread.join()
    
    end_time = time.time()
    execution_time = end_time - start_time
    
    # Собираем статистику
    total_produced = args.producers * args.items
    total_consumed = sum(len(items) for items in all_consumed_items)
    
    # Вычисляем суммы для проверки корректности
    expected_sum = calculate_expected_sum(args.producers, args.items)
    
    # Фактическая сумма потребленных элементов
    consumed_sum = 0
    for items in all_consumed_items:
        consumed_sum += sum(items)
    
    # Проверяем корректность
    all_consumed = []
    for items in all_consumed_items:
        all_consumed.extend(items)
    
    print(f"\n=== Results ===")
    print(f"Execution time: {execution_time:.4f} seconds")
    print(f"Total produced: {total_produced}")
    print(f"Total consumed: {total_consumed}")
    print(f"Expected sum: {expected_sum}")
    print(f"Consumed sum: {consumed_sum}")
    print(f"Buffer operations: produced={buffer.producer_count}, consumed={buffer.consumer_count}")
    
    print(f"\n=== Correctness Check ===")
    count_correct = total_produced == total_consumed
    sum_correct = expected_sum == consumed_sum
    
    print(f"✓ Count check: produced == consumed: {count_correct}")
    print(f"✓ Sum check: expected == actual: {sum_correct}")
    print(f"✓ Overall correctness: {count_correct and sum_correct}")
    
    if not count_correct:
        print(f"  ERROR: Count mismatch! Produced {total_produced}, consumed {total_consumed}")
    if not sum_correct:
        print(f"  ERROR: Sum mismatch! Expected {expected_sum}, got {consumed_sum}")
        print(f"  Difference: {abs(expected_sum - consumed_sum)}")

if __name__ == "__main__":
    main()
