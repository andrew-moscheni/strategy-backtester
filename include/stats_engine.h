#pragma once

#include <cstddef>
#include <cmath>
#include <stdexcept>

class RollingStatistics {
public:
	explicit RollingStatistics(size_t window_size);
	void update(double value);

	double mean() const;
	double variance() const;
	double stddev() const;

	double zscore(double value) const;

	bool is_ready() const;

	size_t count() const;

private:
	size_t window_size_;
	size_t count_;

	double mean_;
	double m2_;
};
