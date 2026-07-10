// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <deque>
#include <future>

DXFCPP_BEGIN_NAMESPACE

/// A simple thread-safe timer.
struct DXFCPP_EXPORT Timer final {
    private:
    std::unique_ptr<std::future<void>> future_;
    std::atomic<bool> isRunning_{};

    Timer() noexcept;

    public:
    void interruptableSleep(std::chrono::milliseconds ms) const;

    void interruptableSleep(std::uint64_t ms) const;

    template <typename F, typename Delay, typename Period>
    static std::shared_ptr<Timer> schedule(F &&f, Delay &&delay, Period &&period) {
        auto t = std::shared_ptr<Timer>(new Timer());

        t->future_ = std::make_unique<std::future<void>>(std::async(
            std::launch::async,
            [self = t](auto &&f, auto &&d, auto &&p) {
                self->isRunning_ = true;
                self->interruptableSleep(d);

                while (self->isRunning_) {
                    f();
                    self->interruptableSleep(p);
                }

                self->isRunning_ = false;
            },
            std::forward<F>(f), std::forward<Delay>(delay), std::forward<Period>(period)));

        return t;
    }

    template <typename F, typename Delay> static std::shared_ptr<Timer> runOnce(F &&f, Delay &&delay) {
        auto t = std::shared_ptr<Timer>(new Timer());

        t->future_ = std::make_unique<std::future<void>>(std::async(
            std::launch::async,
            [self = t](auto &&f, auto &&d) {
                self->isRunning_ = true;
                self->interruptableSleep(d);

                if (self->isRunning_) {
                    f();
                    self->isRunning_ = false;
                }
            },
            std::forward<F>(f), std::forward<Delay>(delay)));

        return t;
    }

    void stop();

    bool isRunning();
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()