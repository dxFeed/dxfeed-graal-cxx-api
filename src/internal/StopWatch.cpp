// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/StopWatch.hpp"

DXFCPP_BEGIN_NAMESPACE

StopWatch::StopWatch() noexcept {
    reset();
}

void StopWatch::start() noexcept {
    if (!isRunning_) {
        std::lock_guard lock{mutex_};
        startTimeStamp_ = std::chrono::steady_clock::now();
        isRunning_ = true;
    }
}

void StopWatch::stop() noexcept {
    if (isRunning_) {
        const auto endTimestamp = std::chrono::steady_clock::now();

        std::lock_guard lock{mutex_};
        const auto elapsedThisPeriod = endTimestamp - startTimeStamp_;

        elapsed_ += std::chrono::duration_cast<std::chrono::milliseconds>(elapsedThisPeriod);
        elapsedInNanos_ += elapsedThisPeriod;
        isRunning_ = false;
    }
}

void StopWatch::reset() noexcept {
    std::lock_guard lock{mutex_};

    elapsed_ = std::chrono::milliseconds::zero();
    elapsedInNanos_ = std::chrono::nanoseconds::zero();
    isRunning_ = false;
    startTimeStamp_ = std::chrono::steady_clock::time_point{};
}

void StopWatch::restart() noexcept {
    std::lock_guard lock{mutex_};

    elapsed_ = std::chrono::milliseconds::zero();
    elapsedInNanos_ = std::chrono::nanoseconds::zero();
    isRunning_ = true;
    startTimeStamp_ = std::chrono::steady_clock::now();
}

bool StopWatch::isRunning() const noexcept {
    return isRunning_;
}

std::chrono::milliseconds StopWatch::elapsed() const noexcept {
    std::lock_guard lock{mutex_};

    auto elapsed = elapsed_;

    if (isRunning_) {
        const auto currentTimestamp = std::chrono::steady_clock::now();
        const auto elapsedUntilNow = currentTimestamp - startTimeStamp_;

        elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(elapsedUntilNow);
    }

    return elapsed;
}

std::chrono::nanoseconds StopWatch::elapsedInNanos() const noexcept {
    std::lock_guard lock{mutex_};

    auto elapsed = elapsedInNanos_;

    if (isRunning_) {
        const auto currentTimestamp = std::chrono::steady_clock::now();
        const auto elapsedUntilNow = currentTimestamp - startTimeStamp_;

        elapsed += elapsedUntilNow;
    }

    return elapsed;
}

DXFCPP_END_NAMESPACE