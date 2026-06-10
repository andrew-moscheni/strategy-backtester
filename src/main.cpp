#include <iostream>
#include <iomanip>
#include "tick.h"
#include "stats_engine.h"

int main() {
    std::string filepath = "../data/BTCUSDT-trades-2024-01-15.csv";

    std::cout << "Loading tick data..." << std::endl;
    auto ticks = parse_csv(filepath);

    if (ticks.empty()) {
        std::cerr << "No ticks loaded." << std::endl;
        return 1;
    }

    RollingStatistics stats(500);

    int anomaly_count = 0;
    const double ANOMALY_THRESHOLD = 3.0;

    std::cout << std::fixed << std::setprecision(2);

    for (const auto& tick : ticks) {

        if (stats.is_ready()) {
            double z = stats.zscore(tick.price);

            if (std::abs(z) > ANOMALY_THRESHOLD) {
                anomaly_count++;

                if (anomaly_count <= 5) {
                    std::cout << "ANOMALY — price: $" << tick.price
                              << "  z-score: "        << z
                              << "  mean: $"          << stats.mean()
                              << "  stddev: $"        << stats.stddev()
                              << std::endl;
                }
            }
        }

        stats.update(tick.price);
    }

    std::cout << "\n--- Summary ---" << std::endl;
    std::cout << "Total ticks processed : " << ticks.size()      << std::endl;
    std::cout << "Final mean price      : $" << stats.mean()     << std::endl;
    std::cout << "Final stddev          : $" << stats.stddev()   << std::endl;
    std::cout << "Anomalies detected    : "  << anomaly_count    << std::endl;

    return 0;
}
