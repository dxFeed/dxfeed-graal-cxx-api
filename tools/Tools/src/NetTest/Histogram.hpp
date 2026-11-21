// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The latency histogram of the NetTest specification: the layout and the percentiles of HdrHistogram 2.1.9 of Java.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dxfcpp::tools::nettest {

/// The lowest discernible value of the specification, ns.
inline constexpr std::int64_t HISTOGRAM_LOWEST = 1;
/// The highest trackable value of the specification, ns (3600 s).
inline constexpr std::int64_t HISTOGRAM_HIGHEST = 3'600'000'000'000;
/// The significant digits of the specification.
inline constexpr int HISTOGRAM_DIGITS = 3;

/// A histogram with the bucket layout of HdrHistogram. Not thread-safe: the owner guards it.
class Histogram {
    std::int64_t highest_;
    int unitMagnitude_;
    int subBucketHalfCountMagnitude_;
    std::int64_t subBucketCount_;
    std::int64_t subBucketHalfCount_;
    std::int64_t subBucketMask_;
    int leadingZeroCountBase_;
    std::vector<std::int64_t> counts_;
    std::int64_t totalCount_ = 0;

    int bucketIndex(std::int64_t value) const;
    std::int64_t subBucketIndex(std::int64_t value, int bucket) const;
    std::size_t countsIndex(std::int64_t value) const;
    std::int64_t valueFromIndex(std::size_t index) const;

    public:
    explicit Histogram(std::int64_t lowest = HISTOGRAM_LOWEST, std::int64_t highest = HISTOGRAM_HIGHEST,
                       int digits = HISTOGRAM_DIGITS);

    /// Records the value `count` times: a negative value as 0, a value above the highest as the highest.
    void record(std::int64_t value, std::int64_t count = 1);

    /// Adds the counts of a histogram of the same layout.
    void add(const Histogram &other);

    void reset();

    std::int64_t totalCount() const {
        return totalCount_;
    }

    /// The lowest value that shares the bucket of the value.
    std::int64_t lowestEquivalentValue(std::int64_t value) const;

    /// The highest value that shares the bucket of the value.
    std::int64_t highestEquivalentValue(std::int64_t value) const;

    /// `getValueAtPercentile` of HdrHistogram 2.1.9: the highest value equivalent to the bucket where the count at the
    /// percentile (rounded to the nearest, at least 1) is reached; the lowest equivalent value for the percentile 0; 0
    /// for an empty histogram.
    std::int64_t valueAtPercentile(double percentile) const;
};

} // namespace dxfcpp::tools::nettest
