#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <cstring>

typedef enum { MODE_UNSYNC, MODE_MUTEX, MODE_ATOMIC } sync_mode_t;

static long long shared_counter_unsync = 0;
static long long shared_counter_mutex = 0;
static std::atomic<long long> shared_counter_atomic(0);
static std::mutex counter_mutex;

struct thread_args_t {
    int thread_index;
    long long iterations_per_thread;
};

static inline long long now_monotonic_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

static void worker_unsync(thread_args_t* arg) {
    for (long long i = 0; i < arg->iterations_per_thread; i++) {
        shared_counter_unsync++;
    }
}

static void worker_mutex(thread_args_t* arg) {
    for (long long i = 0; i < arg->iterations_per_thread; i++) {
        std::lock_guard<std::mutex> lock(counter_mutex);
        shared_counter_mutex++;
    }
}

static void worker_atomic(thread_args_t* arg) {
    for (long long i = 0; i < arg->iterations_per_thread; i++) {
        shared_counter_atomic.fetch_add(1, std::memory_order_relaxed);
    }
}

static sync_mode_t parse_mode(const char* s) {
    if (strcmp(s, "unsync") == 0) return MODE_UNSYNC;
    if (strcmp(s, "mutex") == 0) return MODE_MUTEX;
    if (strcmp(s, "atomic") == 0) return MODE_ATOMIC;
    std::cerr << "Unknown mode: " << s << " (use: unsync|mutex|atomic)" << std::endl;
    exit(2);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <iterations_per_thread> <unsync|mutex|atomic>" << std::endl;
        return 1;
    }

    int num_threads = std::stoi(argv[1]);
    long long iters = std::stoll(argv[2]);
    sync_mode_t mode = parse_mode(argv[3]);

    if (num_threads <= 0 || iters < 0) {
        std::cerr << "Invalid arguments" << std::endl;
        return 1;
    }

    std::cout << "[thread_race] Starting with " << num_threads << " threads, " 
              << iters << " iterations per thread, mode: " << argv[3] << std::endl;

    shared_counter_unsync = 0;
    shared_counter_mutex = 0;
    shared_counter_atomic.store(0);

    std::vector<std::thread> threads;
    std::vector<thread_args_t> args(num_threads);

    long long start_ms = now_monotonic_ms();

    for (int i = 0; i < num_threads; i++) {
        args[i].thread_index = i;
        args[i].iterations_per_thread = iters;
        
        switch (mode) {
            case MODE_UNSYNC:
                threads.emplace_back(worker_unsync, &args[i]);
                break;
            case MODE_MUTEX:
                threads.emplace_back(worker_mutex, &args[i]);
                break;
            case MODE_ATOMIC:
                threads.emplace_back(worker_atomic, &args[i]);
                break;
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    long long end_ms = now_monotonic_ms();
    long long expected = (long long)num_threads * iters;
    long long actual = 0;

    switch (mode) {
        case MODE_UNSYNC:
            actual = shared_counter_unsync;
            break;
        case MODE_MUTEX:
            actual = shared_counter_mutex;
            break;
        case MODE_ATOMIC:
            actual = shared_counter_atomic.load();
            break;
    }

    std::cout << "RESULTS: mode=" << argv[3] 
              << " threads=" << num_threads 
              << " iters_per_thread=" << iters 
              << " expected=" << expected 
              << " actual=" << actual 
              << " time_ms=" << (end_ms - start_ms) 
              << std::endl;

    return 0;
}
