#include "pipeline.h"
#include "tick.h"

#include <iostream>
#include <iomanip>
#include <cmath>

void producer(const std::string& filepath, TickQueue& queue) {
    std::cout << "[producer] Starting — loading " << filepath << std::endl;

    auto ticks = parse_csv(filepath);

    std::cout << "[producer] Pushing " << ticks.size()
              << " ticks into queue..." << std::endl;

    for (const auto& tick : ticks) {
        queue.push(tick);
    }


    queue.set_done();
    std::cout << "[producer] Done." << std::endl;
}

void consumer(TickQueue& queue, RollingStatistics& stats) {
    std::cout << "[consumer] Starting." << std::endl;

    int    anomaly_count = 0;
    size_t tick_count    = 0;
    const double THRESHOLD = 3.0;

    std::cout << std::fixed << std::setprecision(2);

    while (true) {
        std::optional<Tick> maybe_tick = queue.pop();

        if (!maybe_tick.has_value()) {
            break;
        }

        const Tick& tick = maybe_tick.value();
        tick_count++;


        if (stats.is_ready()) {
            double z = stats.zscore(tick.price);

            if (std::abs(z) > THRESHOLD) {
                anomaly_count++;
                if (anomaly_count <= 5) {
                    std::cout << "[consumer] ANOMALY #" << anomaly_count
                              << " — price: $"  << tick.price
                              << "  z-score: "  << z
                              << "  mean: $"    << stats.mean()
                              << "  stddev: $"  << stats.stddev()
                              << std::endl;
                }
            }
        }

        stats.update(tick.price);
    }

    std::cout << "\n[consumer] Done." << std::endl;
    std::cout << "--- Summary ---"                          << std::endl;
    std::cout << "Ticks processed : " << tick_count        << std::endl;
    std::cout << "Final mean      : $" << stats.mean()     << std::endl;
    std::cout << "Final stddev    : $" << stats.stddev()   << std::endl;
    std::cout << "Anomalies found : " << anomaly_count     << std::endl;
}
