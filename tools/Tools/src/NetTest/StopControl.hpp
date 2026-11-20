// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The stop flag of a run: set once, from any thread, and waited on with a timeout, so that every wait of the tool wakes
// the moment the stop is requested.

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace dxfcpp::tools::nettest {

class StopControl {
    std::atomic<bool> stopped_{false};
    std::mutex mutex_{};
    std::condition_variable condition_{};

  public:
    StopControl() = default;
    StopControl(const StopControl &) = delete;
    StopControl &operator=(const StopControl &) = delete;

    bool stopped() const {
        return stopped_.load(std::memory_order_acquire);
    }

    /// Sets the flag and wakes every waiter; later calls change nothing. Not for signal handlers.
    void request() {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            stopped_.store(true, std::memory_order_release);
        }

        condition_.notify_all();
    }

    /// Waits until the stop is requested or the timeout passes; returns true when the stop was requested.
    template <typename Rep, typename Period> bool waitFor(const std::chrono::duration<Rep, Period> &timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        return condition_.wait_for(lock, timeout, [this] {
            return stopped();
        });
    }

    /// Waits until the stop is requested or the time of the wall clock comes; returns true when the stop was
    /// requested.
    bool waitUntil(const std::chrono::system_clock::time_point &time) {
        std::unique_lock<std::mutex> lock(mutex_);

        return condition_.wait_until(lock, time, [this] {
            return stopped();
        });
    }

    /// Waits until the stop is requested.
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this] {
            return stopped();
        });
    }
};

} // namespace dxfcpp::tools::nettest
