// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <mutex>

DXFCPP_BEGIN_NAMESPACE

struct StopWatch final {
    private:
    mutable std::mutex mutex_{};
    std::chrono::milliseconds elapsed_{};
    std::chrono::steady_clock::time_point startTimeStamp_{};
    std::chrono::steady_clock::time_point elapsedInTimePoints_{};
    std::atomic<bool> isRunning_{};

    public:
    StopWatch() noexcept {
        reset();
    }

    void start() noexcept {
        if (!isRunning_) {
            std::lock_guard lock{mutex_};
            startTimeStamp_ = std::chrono::steady_clock::now();
            isRunning_ = true;
        }
    }

    void stop() noexcept {
        if (isRunning_) {
            const auto endTimestamp = std::chrono::steady_clock::now();

            std::lock_guard lock{mutex_};
            const auto elapsedThisPeriod = endTimestamp - startTimeStamp_;

            elapsed_ += std::chrono::duration_cast<std::chrono::milliseconds>(elapsedThisPeriod);
            elapsedInTimePoints_ += elapsedThisPeriod;
            isRunning_ = false;
        }
    }

    void reset() noexcept {
        std::lock_guard lock{mutex_};

        elapsed_ = std::chrono::milliseconds::zero();
        elapsedInTimePoints_ = std::chrono::steady_clock::time_point{};
        isRunning_ = false;
        startTimeStamp_ = std::chrono::steady_clock::time_point{};
    }

    void restart() noexcept {
        std::lock_guard lock{mutex_};

        elapsed_ = std::chrono::milliseconds::zero();
        elapsedInTimePoints_ = std::chrono::steady_clock::time_point{};
        isRunning_ = true;
        startTimeStamp_ = std::chrono::steady_clock::now();
    }

    bool isRunning() const noexcept {
        return isRunning_;
    }

    std::chrono::milliseconds elapsed() const noexcept {
        std::lock_guard lock{mutex_};

        auto elapsed = elapsed_;

        if (isRunning_) {
            const auto currentTimestamp = std::chrono::steady_clock::now();
            const auto elapsedUntilNow = currentTimestamp - startTimeStamp_;

            elapsed += std::chrono::duration_cast<std::chrono::milliseconds>(elapsedUntilNow);
        }

        return elapsed;
    }

    std::chrono::steady_clock::time_point elapsedInTimePoints() const noexcept {
        std::lock_guard lock{mutex_};

        auto elapsed = elapsedInTimePoints_;

        if (isRunning_) {
            const auto currentTimestamp = std::chrono::steady_clock::now();
            const auto elapsedUntilNow = currentTimestamp - startTimeStamp_;

            elapsed += elapsedUntilNow;
        }

        return elapsed;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()