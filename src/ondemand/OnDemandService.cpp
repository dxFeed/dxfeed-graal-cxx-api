// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

std::shared_ptr<OnDemandService> OnDemandService::create(void *feedHandle) noexcept {
    return {};
}

OnDemandService::OnDemandService() noexcept : handle_{} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("    OnDemandService() noexcept : handle_{} {()");
    }
}

OnDemandService::~OnDemandService() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("OnDemandService{" + handle_.toString() + "}::~OnDemandService()");
    }
}

std::shared_ptr<OnDemandService> OnDemandService::getInstance() noexcept {
    return {};
}

std::shared_ptr<OnDemandService> OnDemandService::getInstance(std::shared_ptr<DXEndpoint> endpoint) noexcept {
    return {};
}

std::shared_ptr<DXEndpoint> OnDemandService::getEndpoint() const noexcept {
    return endpoint_;
}

bool OnDemandService::isReplaySupported() const noexcept {
    return false;
}

bool OnDemandService::isReplay() const noexcept {
    return false;
}

bool OnDemandService::isClear() const noexcept {
    return false;
}

std::int64_t OnDemandService::getTime() const noexcept {
    return {};
}

double OnDemandService::getSpeed() const noexcept {
    return {};
}

void OnDemandService::replay(std::int64_t time) const noexcept {
    return;
}

void OnDemandService::replay(std::int64_t time, double speed) const noexcept {
    return;
}

void OnDemandService::pause() const noexcept {
    return;
}

void OnDemandService::stopAndResume() const noexcept {
    return;
}

void OnDemandService::stopAndClear() const noexcept {
    return;
}

void OnDemandService::setSpeed(double speed) const noexcept {
    return;
}

}