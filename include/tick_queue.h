#pragma once

#include "tick.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

class TickQueue {
public:
	explicit TickQueue(size_t max_size);
	void push(const Tick& tick);

	std::optional<Tick> pop();
	void set_done();

private:
	size_t max_size_;
	std::queue<Tick> queue_;
	std::mutex mutex_;
	std::condition_variable cv_;
	bool done_;
};
