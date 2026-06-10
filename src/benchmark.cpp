#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <numeric>

#include "tick.h"
#include "stats_engine.h"
#include "tick_queue.h"
#include "pipeline.h"

static std::vector<Tick> make_synthetic_ticks(size_t n) {
    std::vector<Tick> ticks;
    ticks.reserve(n);

    for (size_t i = 0; i < n; i++) {
        Tick t;
        t.timestamp_ms = static_cast<int64_t>(i);
        t.price    = 42000.0 + (i % 200) * 0.5;
        t.quantity = 0.01;
        ticks.push_back(t);
    }
    return ticks;
}

static void bench_stats_engine(size_t n) {
    auto ticks = make_synthetic_ticks(n);
    RollingStatistics stats(500);

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& tick : ticks) {
        stats.update(tick.price);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double us = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();

    double ticks_per_sec = (n / us) * 1e6;

    std::cout << std::fixed << std::setprecision(0);
    std::cout << "[bench] Stats engine (single-threaded)\n";
    std::cout << "        Ticks      : " << n << "\n";
    std::cout << "        Time       : " << std::setprecision(2)
              << us / 1000.0 << " ms\n";
    std::cout << "        Throughput : " << std::setprecision(0)
              << ticks_per_sec << " ticks/sec\n\n";
}

static void bench_pipeline(size_t n) {
    auto ticks = make_synthetic_ticks(n);

    TickQueue    queue(1000);
    RollingStatistics stats(500);

    auto start = std::chrono::high_resolution_clock::now();

    std::thread prod([&] {
        for (const auto& tick : ticks) {
            queue.push(tick);
        }
        queue.set_done();
    });

    std::thread cons([&] {
        while (true) {
            auto t = queue.pop();
            if (!t.has_value()) break;
            stats.update(t->price);
        }
    });

    prod.join();
    cons.join();

    auto end = std::chrono::high_resolution_clock::now();

    double us = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();

    double ticks_per_sec = (n / us) * 1e6;

    std::cout << "[bench] Full pipeline (producer + consumer threads)\n";
    std::cout << "        Ticks      : " << n << "\n";
    std::cout << "        Time       : " << std::setprecision(2)
              << us / 1000.0 << " ms\n";
    std::cout << "        Throughput : " << std::setprecision(0)
              << ticks_per_sec << " ticks/sec\n\n";
}

int main() {
    std::cout << "=== Tick Processor Benchmark ===\n\n";

    const size_t N = 1'000'000;

    bench_stats_engine(N);
    bench_pipeline(N);

    std::cout << "================================\n";
    return 0;
}
