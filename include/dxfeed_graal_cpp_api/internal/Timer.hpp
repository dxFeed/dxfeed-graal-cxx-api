// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "Common.hpp"

DXFCPP_BEGIN_NAMESPACE

struct Timer {
  private:
    std::unique_ptr<std::future<void>> future_;

    std::atomic<bool> isRunning_{};

    Timer() noexcept = default;

  public:
    template <typename F, typename Delay, typename Period>
    static std::shared_ptr<Timer> schedule(F &&f, Delay &&delay, Period &&period) {
        auto t = std::shared_ptr<Timer>(new Timer());

        t->future_ = std::make_unique<std::future<void>>(std::async(
            std::launch::async,
            [self = t](auto &&f, auto &&d, auto &&p) {
                self->isRunning_ = true;
                std::this_thread::sleep_for(d);

                while (self->isRunning_) {
                    f();
                    std::this_thread::sleep_for(p);
                }
            },
            std::forward<F>(f), std::forward<Delay>(delay), std::forward<Period>(period)));

        return t;
    }

    void stop() {
        isRunning_ = false;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()