// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Histogram.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dxfcpp::tools::nettest {

namespace {

// Long.numberOfLeadingZeros of Java for a value that is not 0.
int leadingZeros(std::uint64_t value) {
    int count = 0;

    for (const int shift : {32, 16, 8, 4, 2, 1}) {
        if ((value >> (64 - shift)) == 0) {
            count += shift;
            value <<= shift;
        }
    }

    return count;
}

// The smallest exponent m with 2^m >= value, for a value of at least 1.
int ceilLog2(std::int64_t value) {
    int exponent = 0;

    while ((std::int64_t{1} << exponent) < value) {
        ++exponent;
    }

    return exponent;
}

} // namespace

Histogram::Histogram(std::int64_t lowest, std::int64_t highest, int digits) : highest_(highest) {
    std::int64_t largestValueWithSingleUnitResolution = 2;

    for (int digit = 0; digit < digits; ++digit) {
        largestValueWithSingleUnitResolution *= 10;
    }

    unitMagnitude_ = 63 - leadingZeros(static_cast<std::uint64_t>(std::max<std::int64_t>(lowest, 1)));

    const int subBucketCountMagnitude = ceilLog2(largestValueWithSingleUnitResolution);

    subBucketHalfCountMagnitude_ = std::max(subBucketCountMagnitude, 1) - 1;
    subBucketCount_ = std::int64_t{1} << (subBucketHalfCountMagnitude_ + 1);
    subBucketHalfCount_ = subBucketCount_ / 2;
    subBucketMask_ = (subBucketCount_ - 1) << unitMagnitude_;
    leadingZeroCountBase_ = 64 - unitMagnitude_ - subBucketHalfCountMagnitude_ - 1;

    // getBucketsNeededToCoverValue of HdrHistogram.
    std::int64_t smallestUntrackableValue = subBucketCount_ << unitMagnitude_;
    std::int64_t bucketCount = 1;

    while (smallestUntrackableValue <= highest) {
        if (smallestUntrackableValue > std::numeric_limits<std::int64_t>::max() / 2) {
            ++bucketCount;

            break;
        }

        smallestUntrackableValue <<= 1;
        ++bucketCount;
    }

    counts_.assign(static_cast<std::size_t>((bucketCount + 1) * subBucketHalfCount_), 0);
}

int Histogram::bucketIndex(std::int64_t value) const {
    return leadingZeroCountBase_ - leadingZeros(static_cast<std::uint64_t>(value | subBucketMask_));
}

std::int64_t Histogram::subBucketIndex(std::int64_t value, int bucket) const {
    return value >> (bucket + unitMagnitude_);
}

std::size_t Histogram::countsIndex(std::int64_t value) const {
    const int bucket = bucketIndex(value);
    const auto bucketBaseIndex = static_cast<std::int64_t>(bucket + 1) << subBucketHalfCountMagnitude_;

    return static_cast<std::size_t>(bucketBaseIndex + subBucketIndex(value, bucket) - subBucketHalfCount_);
}

std::int64_t Histogram::valueFromIndex(std::size_t index) const {
    int bucket = static_cast<int>(index >> subBucketHalfCountMagnitude_) - 1;
    auto subBucket =
        static_cast<std::int64_t>(index & static_cast<std::size_t>(subBucketHalfCount_ - 1)) + subBucketHalfCount_;

    if (bucket < 0) {
        subBucket -= subBucketHalfCount_;
        bucket = 0;
    }

    return subBucket << (bucket + unitMagnitude_);
}

void Histogram::record(std::int64_t value, std::int64_t count) {
    const auto clamped = std::min(std::max<std::int64_t>(value, 0), highest_);

    counts_[countsIndex(clamped)] += count;
    totalCount_ += count;
}

void Histogram::add(const Histogram &other) {
    const auto size = std::min(counts_.size(), other.counts_.size());

    for (std::size_t index = 0; index < size; ++index) {
        counts_[index] += other.counts_[index];
    }

    totalCount_ += other.totalCount_;
}

void Histogram::reset() {
    std::fill(counts_.begin(), counts_.end(), 0);
    totalCount_ = 0;
}

std::int64_t Histogram::lowestEquivalentValue(std::int64_t value) const {
    const int bucket = bucketIndex(value);

    return subBucketIndex(value, bucket) << (bucket + unitMagnitude_);
}

std::int64_t Histogram::highestEquivalentValue(std::int64_t value) const {
    const int bucket = bucketIndex(value);
    const int adjustedBucket = subBucketIndex(value, bucket) >= subBucketCount_ ? bucket + 1 : bucket;
    const auto size = std::int64_t{1} << (unitMagnitude_ + adjustedBucket);

    return lowestEquivalentValue(value) + size - 1;
}

std::int64_t Histogram::valueAtPercentile(double percentile) const {
    const double requested = std::min(percentile, 100.0);
    // Rounded to the nearest count, as HdrHistogram 2.1.9 does.
    auto countAtPercentile = static_cast<std::int64_t>(requested / 100.0 * static_cast<double>(totalCount_) + 0.5);

    countAtPercentile = std::max<std::int64_t>(countAtPercentile, 1);

    std::int64_t total = 0;

    for (std::size_t index = 0; index < counts_.size(); ++index) {
        total += counts_[index];

        if (total >= countAtPercentile) {
            const auto value = valueFromIndex(index);

            // percentile == 0.0, without the warning of an exact floating-point comparison.
            return std::fpclassify(percentile) == FP_ZERO ? lowestEquivalentValue(value)
                                                          : highestEquivalentValue(value);
        }
    }

    return 0;
}

} // namespace dxfcpp::tools::nettest
