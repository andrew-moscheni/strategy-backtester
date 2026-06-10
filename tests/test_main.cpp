#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <fstream>
#include <cmath>

#include "tick.h"
#include "stats_engine.h"
#include "tick_queue.h"

// ─── Parser tests ─────────────────────────────────────────────────────────────

TEST(ParserTest, ParsesValidRows) {
    std::ofstream tmp("/tmp/test_ticks.csv");
    tmp << "1,42580.01,0.00500000,212.90,1705276800017,true,true\n";
    tmp << "2,42600.00,0.01200000,511.20,1705276800035,false,true\n";
    tmp << "3,42550.50,0.00800000,340.40,1705276800051,true,false\n";
    tmp.close();

    auto ticks = parse_csv("/tmp/test_ticks.csv");

    EXPECT_EQ(ticks.size(), 3u);
    EXPECT_DOUBLE_EQ(ticks[0].price, 42580.01);
    EXPECT_DOUBLE_EQ(ticks[1].quantity, 0.012);
    EXPECT_EQ(ticks[2].timestamp_ms, 1705276800051);
}

TEST(ParserTest, SkipsMalformedRows) {
    std::ofstream tmp("/tmp/test_bad.csv");
    tmp << "1,42580.01,0.005,212.90,1705276800017,true,true\n";
    tmp << "this,is,bad\n";
    tmp << "3,42600.00,0.012,511.20,1705276800035,false,true\n";
    tmp.close();

    auto ticks = parse_csv("/tmp/test_bad.csv");
    EXPECT_EQ(ticks.size(), 2u);
}

TEST(ParserTest, ReturnsEmptyOnMissingFile) {
    EXPECT_THROW(parse_csv("/tmp/does_not_exist_xyz.csv"), std::runtime_error);
}

TEST(ParserTest, HandlesEmptyFile) {
    std::ofstream tmp("/tmp/test_empty.csv");
    tmp.close();

    auto ticks = parse_csv("/tmp/test_empty.csv");
    EXPECT_EQ(ticks.size(), 0u);
}

// ─── Stats engine tests ───────────────────────────────────────────────────────

TEST(StatsEngineTest, MeanIsCorrect) {
    RollingStatistics stats(3);
    stats.update(10.0);
    stats.update(20.0);
    stats.update(30.0);
    EXPECT_NEAR(stats.mean(), 20.0, 1e-9);
}

TEST(StatsEngineTest, StddevIsCorrect) {
    RollingStatistics stats(4);
    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        stats.update(v);
    }
    EXPECT_NEAR(stats.stddev(), 2.0, 1e-6);
}

TEST(StatsEngineTest, ZscoreDetectsAnomaly) {
    RollingStatistics stats(5);
    for (int i = 0; i < 100; i++) {
        stats.update(i % 2 == 0 ? 99.0 : 101.0);
    }
    double z = stats.zscore(200.0);
    EXPECT_GT(z, 3.0);
}

TEST(StatsEngineTest, ZscoreIsZeroWhenStddevIsZero) {
    RollingStatistics stats(3);
    for (int i = 0; i < 10; i++) stats.update(50.0);
    EXPECT_DOUBLE_EQ(stats.zscore(999.0), 0.0);
}

TEST(StatsEngineTest, NotReadyUntilWindowFilled) {
    RollingStatistics stats(10);
    for (int i = 0; i < 9; i++) {
        stats.update(1.0);
        EXPECT_FALSE(stats.is_ready());
    }
    stats.update(1.0);
    EXPECT_TRUE(stats.is_ready());
}

TEST(StatsEngineTest, CountTracksUpdates) {
    RollingStatistics stats(2);
    EXPECT_EQ(stats.count(), 0u);
    stats.update(1.0);
    EXPECT_EQ(stats.count(), 1u);
    stats.update(2.0);
    EXPECT_EQ(stats.count(), 2u);
}

TEST(StatsEngineTest, ThrowsOnWindowSizeLessThanTwo) {
    EXPECT_THROW(RollingStatistics(1), std::runtime_error);
    EXPECT_THROW(RollingStatistics(0), std::runtime_error);
}

TEST(StatsEngineTest, MeanUpdatesIncrementally) {
    RollingStatistics stats(2);
    stats.update(10.0);
    EXPECT_NEAR(stats.mean(), 10.0, 1e-9);
    stats.update(20.0);
    EXPECT_NEAR(stats.mean(), 15.0, 1e-9);
    stats.update(30.0);
    EXPECT_NEAR(stats.mean(), 20.0, 1e-9);
}

// ─── Queue tests ──────────────────────────────────────────────────────────────

TEST(TickQueueTest, ProducerConsumerDeliversAllTicks) {
    TickQueue queue(10);
    const int NUM_TICKS = 1000;
    std::atomic<int> received(0);

    std::thread prod([&] {
        for (int i = 0; i < NUM_TICKS; i++) {
            Tick t{ static_cast<int64_t>(i), static_cast<double>(i), 1.0 };
            queue.push(t);
        }
        queue.set_done();
    });

    std::thread cons([&] {
        while (true) {
            auto tick = queue.pop();
            if (!tick.has_value()) break;
            received++;
        }
    });

    prod.join();
    cons.join();

    EXPECT_EQ(received.load(), NUM_TICKS);
}

TEST(TickQueueTest, PopReturnsNulloptWhenDoneAndEmpty) {
    TickQueue queue(10);
    queue.set_done();
    auto result = queue.pop();
    EXPECT_FALSE(result.has_value());
}

TEST(TickQueueTest, PreservesTickValues) {
    TickQueue queue(5);

    Tick original{ 1705276800017LL, 42580.01, 0.005 };
    queue.push(original);
    queue.set_done();

    auto result = queue.pop();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->timestamp_ms, original.timestamp_ms);
    EXPECT_DOUBLE_EQ(result->price,  original.price);
    EXPECT_DOUBLE_EQ(result->quantity, original.quantity);
}

// ─── Test runner ──────────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
