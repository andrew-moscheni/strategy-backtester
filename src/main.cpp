#include <iostream>
#include <thread>
#include "tick_queue.h"
#include "stats_engine.h"
#include "pipeline.h"

int main() {
    std::string filepath = "../data/BTCUSDT-trades-2024-01-15.csv";

    TickQueue queue(1000);

    RollingStatistics stats(500);

    std::thread producer_thread([&] {
        producer(filepath, queue);
    });

    std::thread consumer_thread([&] {
        consumer(queue, stats);
    });
    producer_thread.join();
    consumer_thread.join();

    std::cout << "\nPipeline complete." << std::endl;
    return 0;
}
