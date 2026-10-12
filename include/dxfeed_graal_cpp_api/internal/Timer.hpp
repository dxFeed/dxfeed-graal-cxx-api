// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <deque>
#include <future>
#include <thread>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

/// A simple thread-safe timer.
struct DXFCPP_EXPORT Timer final {
    private:
    // Not used: the task runs on a detached thread. A std::async future here kept the timer alive forever (the future
    // holds the task, the task holds the timer) and could not be released on the timer's own thread. Kept for the ABI.
    std::unique_ptr<std::future<void>> future_;
    std::atomic<bool> isRunning_{};

    Timer() noexcept;

    // A delay or a period: milliseconds as an integer, or a std::chrono duration.
    template <typename Duration> static std::chrono::milliseconds toMilliseconds(const Duration &duration) {
        if constexpr (std::is_integral_v<Duration>) {
            return std::chrono::milliseconds{duration};
        } else {
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        }
    }

    // An exception of the task is not propagated (it was kept in the never-read std::async future); a periodic task
    // stops after it, as before. Returns false on an exception.
    template <typename F> static bool runTask(F &function) noexcept {
        try {
            function();

            return true;
        } catch (...) {
            return false;
        }
    }

    public:
    void interruptableSleep(std::chrono::milliseconds ms) const;

    void interruptableSleep(std::uint64_t ms) const;

    template <typename F, typename Delay, typename Period>
    static std::shared_ptr<Timer> schedule(F &&f, Delay &&delay, Period &&period) {
        auto t = std::shared_ptr<Timer>(new Timer());

        // Set before the thread starts, so that stop() called right after schedule() is not lost.
        t->isRunning_ = true;
        // The thread holds the timer until it ends; the timer is freed when the caller has released it too.
        std::thread([self = t, function = std::forward<F>(f), d = toMilliseconds(delay),
                     p = toMilliseconds(period)]() mutable {
            self->interruptableSleep(d);

            while (self->isRunning_) {
                if (!runTask(function)) {
                    break;
                }

                self->interruptableSleep(p);
            }

            self->isRunning_ = false;
        }).detach();

        return t;
    }

    template <typename F, typename Delay> static std::shared_ptr<Timer> runOnce(F &&f, Delay &&delay) {
        auto t = std::shared_ptr<Timer>(new Timer());

        // Set before the thread starts, so that stop() called right after runOnce() is not lost.
        t->isRunning_ = true;
        // The thread holds the timer until it ends; the timer is freed when the caller has released it too.
        std::thread([self = t, function = std::forward<F>(f), d = toMilliseconds(delay)]() mutable {
            self->interruptableSleep(d);

            if (self->isRunning_) {
                runTask(function);
                self->isRunning_ = false;
            }
        }).detach();

        return t;
    }

    void stop();

    bool isRunning();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()