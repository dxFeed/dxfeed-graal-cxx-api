// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "InternalExport.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <atomic>
#include <chrono>
#include <mutex>

DXFCPP_BEGIN_NAMESPACE

/// A simple thread-safe stopwatch.
struct DXFCPP_INTERNAL_EXPORT StopWatch final {
    private:
    mutable std::mutex mutex_{};
    std::chrono::milliseconds elapsed_{};
    std::chrono::steady_clock::time_point startTimeStamp_{};
    std::chrono::nanoseconds elapsedInNanos_{};
    std::atomic<bool> isRunning_{};

    public:
    StopWatch() noexcept;

    void start() noexcept;

    void stop() noexcept;

    void reset() noexcept;

    void restart() noexcept;

    bool isRunning() const noexcept;

    std::chrono::milliseconds elapsed() const noexcept;

    std::chrono::nanoseconds elapsedInNanos() const noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()