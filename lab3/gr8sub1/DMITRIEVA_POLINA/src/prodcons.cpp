#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct ring_buffer_t {
    std::vector<int> data;
    int capacity;
    int head;
    int tail;
    int count;
    std::mutex mutex;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    std::atomic<int> producers_active;
};

void rb_init(ring_buffer_t* rb, int capacity, int producers_total) {
    rb->data.resize(capacity);
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->producers_active.store(producers_total);
}

void rb_push(ring_buffer_t* rb, int value) {
    std::unique_lock<std::mutex> lock(rb->mutex);
    rb->not_full.wait(lock, [rb]() { return rb->count < rb->capacity; });
    rb->data[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    rb->not_empty.notify_one();
}

bool rb_pop(ring_buffer_t* rb, int* value) {
    std::unique_lock<std::mutex> lock(rb->mutex);
    rb->not_empty.wait(lock, [rb]() { 
        return rb->count > 0 || rb->producers_active.load() == 0; 
    });
    if (rb->count == 0 && rb->producers_active.load() == 0) {
        return false;
    }
    *value = rb->data[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    rb->not_full.notify_one();
    return true;
}

void rb_producer_done(ring_buffer_t* rb) {
    int remaining = rb->producers_active.fetch_sub(1) - 1;
    if (remaining == 0) {
        std::unique_lock<std::mutex> lock(rb->mutex);
        rb->not_empty.notify_all();
    }
}

void producer_thread(ring_buffer_t* rb, int items_to_produce, int producer_index) {
    for (int i = 0; i < items_to_produce; i++) {
        int value = (producer_index + 1) * 1000000 + i;
        rb_push(rb, value);
    }
    rb_producer_done(rb);
}

void consumer_thread(ring_buffer_t* rb, long long* consumed_sum, long long* consumed_count) {
    int value;
    while (rb_pop(rb, &value)) {
        *consumed_sum += value;
        *consumed_count += 1;
    }
}

int main() {
    int P = 2, C = 2, B = 64;
    long long N = 100000;

    std::cout << "[prodcons] Starting with P=" << P << " C=" << C 
              << " N=" << N << " B=" << B << std::endl;

    ring_buffer_t rb;
    rb_init(&rb, B, P);

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    std::vector<long long> consumer_sums(C, 0);
    std::vector<long long> consumer_counts(C, 0);

    int per_producer = static_cast<int>(N / P);
    int remainder = static_cast<int>(N % P);

    for (int i = 0; i < P; i++) {
        int items = per_producer + (i < remainder ? 1 : 0);
        producers.emplace_back(producer_thread, &rb, items, i);
    }

    for (int i = 0; i < C; i++) {
        consumers.emplace_back(consumer_thread, &rb, &consumer_sums[i], &consumer_counts[i]);
    }

    for (auto& t : producers) {
        t.join();
    }

    for (auto& t : consumers) {
        t.join();
    }

    long long produced_total = N;
    long long consumed_total = 0;
    long long consumed_sum = 0;
    
    for (int i = 0; i < C; i++) {
        consumed_total += consumer_counts[i];
        consumed_sum += consumer_sums[i];
    }

    std::cout << "RESULTS: P=" << P << " C=" << C << " N=" << N << " B=" << B
              << " produced=" << produced_total 
              << " consumed=" << consumed_total 
              << " sum=" << consumed_sum 
              << std::endl;

    if (produced_total == consumed_total) {
        std::cout << "✓ CORRECT: produced == consumed" << std::endl;
    } else {
        std::cout << "✗ ERROR: produced != consumed" << std::endl;
    }

    return 0;
}
