#include "stats_engine.h"

RollingStatistics::RollingStatistics(size_t window_size)
	: window_size_(window_size)
	, count_(0)
	, mean_(0.0)
	, m2_(0.0)
{
	if (window_size < 2) {
		throw std::runtime_error("Window size must be at least 2");
	}
}

void RollingStatistics::update(double value) {
	count_++;

	// use Welford's update to update mean and variance
	double delta = value-mean_;

	mean_ += delta/static_cast<double>(count_);

	double delta2 = value-mean_;
	m2_ += delta * delta2;
}

double RollingStatistics::mean() const {
	return mean_;
}

double RollingStatistics::variance() const {
	if (count_<2) return 0.0;
	return m2_ / static_cast<double>(count_);
}

double RollingStatistics::stddev() const {
	return std::sqrt(variance());
}

double RollingStatistics::zscore(double value) const {
	double sd = stddev();
	
	if (sd < 1e-10) return 0.0;

	return (value-mean_) / sd;
}

bool RollingStatistics::is_ready() const {
	return count_ >= window_size_;
}

size_t RollingStatistics::count() const {
	return count_;
}
