#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "allocator.h"
#include "allocator_slab.h"

using Clock = std::chrono::high_resolution_clock;

constexpr size_t ITERATIONS = 5'000'000;

volatile void* sink;

template <typename Func>
void run_benchmark(const std::string& name, Func func) {
    // Warmup
    for (size_t i = 0; i < 10000; ++i) func();

    auto start = Clock::now();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        func();
        std::atomic_signal_fence(std::memory_order_seq_cst);
    }

    auto end = Clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    double ns_per_op = (double)duration.count() / ITERATIONS;
    double ops_per_sec = 1e9 / ns_per_op;

    std::cout << name << "\n";
    std::cout << "  Total time: " << duration.count() / 1e6 << " ms\n";
    std::cout << "  Latency:    " << ns_per_op << " ns/op\n";
    std::cout << "  Throughput: " << ops_per_sec / 1e6 << " M ops/sec\n\n";
}

void bench_malloc() {
    void* p = std::malloc(128);
    sink = p;  // prevent optimization
    std::free(p);
}

void bench_pool_mutex(Allocator& alloc) {
    void* p = alloc.allocate();
    sink = p;
    alloc.free(p);
}

void bench_pool_tls() {
    thread_local Allocator alloc(128, 100);
    void* p = alloc.allocate();
    sink = p;
    alloc.free(p);
}

void bench_slab(SlabAllocator& alloc) {
    void* p = alloc.allocate(100);
    sink = p;
    alloc.free(p, 100);
}

int main() {
    Allocator pool_alloc(128, 100);
    SlabAllocator slab_alloc;

    run_benchmark("malloc/free", [] { bench_malloc(); });

    run_benchmark("pool allocator (mutex)", [&] { bench_pool_mutex(pool_alloc); });

    run_benchmark("pool allocator (TLS)", [] { bench_pool_tls(); });

    run_benchmark("slab allocator", [&] { bench_slab(slab_alloc); });

    return 0;
}
