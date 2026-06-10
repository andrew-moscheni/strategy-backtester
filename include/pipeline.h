#pragma once

#include "tick_queue.h"
#include "stats_engine.h"
#include <string>

void producer(const std::string& filepath, TickQueue& queue);

void consumer(TickQueue& queue, RollingStatistics& stats);
