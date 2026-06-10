#include "tick_queue.h"

TickQueue::TickQueue(size_t max_size)
	: max_size_(max_size)
	, done_(false)
{}

void TickQueue::push(const Tick& tick) {
	std::unique_lock<std::mutex> lock(mutex_);

	cv_.wait(lock, [this] {
		return queue_.size() < max_size_;
	});

	queue_.push(tick);
	cv_.notify_one();
}

std::optional<Tick> TickQueue::pop() {
	std::unique_lock<std::mutex> lock(mutex_);
	cv_.wait(lock, [this] {
		return !queue_.empty() || done_;
	});

	if (queue_.empty() && done_) {
		return std::nullopt;
	}

	Tick tick=queue_.front();
	queue_.pop();

	cv_.notify_one();

	return tick;
}

void TickQueue::set_done() {
	{
		std::unique_lock<std::mutex> lock(mutex_);
		done_ = true;
	}

	cv_.notify_all();
}
