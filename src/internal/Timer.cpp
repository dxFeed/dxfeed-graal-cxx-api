// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Timer.hpp"

DXFCPP_BEGIN_NAMESPACE

Timer::Timer() noexcept {
}
void Timer::interruptableSleep(std::chrono::milliseconds ms) const {
    constexpr auto MIN_SLEEP = std::chrono::milliseconds(10);
    const auto startTimeStamp = std::chrono::steady_clock::now();

    while (isRunning_ && std::chrono::steady_clock::now() - startTimeStamp < ms) {
        std::this_thread::sleep_for(MIN_SLEEP);
    }
}

void Timer::interruptableSleep(std::uint64_t ms) const {
    interruptableSleep(std::chrono::milliseconds(ms));
}

void Timer::stop() {
    isRunning_ = false;
}

bool Timer::isRunning() {
    return isRunning_;
}

DXFCPP_END_NAMESPACE