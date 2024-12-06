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

DXFCPP_BEGIN_NAMESPACE

OnDemandService::OnDemandService() noexcept : handle_{} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("OnDemandService()");
    }
}

OnDemandService::~OnDemandService() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("OnDemandService{" + handle_.toString() + "}::~OnDemandService()");
    }
}

std::shared_ptr<OnDemandService> OnDemandService::getInstance() {
    return getInstance(DXEndpoint::getInstance(DXEndpoint::Role::ON_DEMAND_FEED));
}

std::shared_ptr<OnDemandService> OnDemandService::getInstance(std::shared_ptr<DXEndpoint> endpoint) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("OnDemandService::getInstance(" + endpoint->toString() + ")");
    }

    std::shared_ptr<OnDemandService> onDemandService{new OnDemandService{}};

    auto id = ApiContext::getInstance()->getManager<EntityManager<OnDemandService>>()->registerEntity(onDemandService);
    ignoreUnused(id);

    onDemandService->endpoint_ = endpoint;
    onDemandService->handle_ = isolated::ondemand::IsolatedOnDemandService::getInstance(endpoint->handle_);

    return onDemandService;
}

std::shared_ptr<DXEndpoint> OnDemandService::getEndpoint() const noexcept {
    return endpoint_;
}

bool OnDemandService::isReplaySupported() const {
    return isolated::ondemand::IsolatedOnDemandService::isReplaySupported(handle_);
}

bool OnDemandService::isReplay() const {
    return isolated::ondemand::IsolatedOnDemandService::isReplay(handle_);
}

bool OnDemandService::isClear() const {
    return isolated::ondemand::IsolatedOnDemandService::isClear(handle_);
}

std::int64_t OnDemandService::getTime() const {
    return isolated::ondemand::IsolatedOnDemandService::getTime(handle_);
}

double OnDemandService::getSpeed() const {
    return isolated::ondemand::IsolatedOnDemandService::getSpeed(handle_);
}

void OnDemandService::replay(std::int64_t time) const {
    isolated::ondemand::IsolatedOnDemandService::replay(handle_, time);
}

void OnDemandService::replay(std::int64_t time, double speed) const {
    isolated::ondemand::IsolatedOnDemandService::replay(handle_, time, speed);
}

void OnDemandService::pause() const {
    isolated::ondemand::IsolatedOnDemandService::pause(handle_);
}

void OnDemandService::stopAndResume() const {
    isolated::ondemand::IsolatedOnDemandService::stopAndResume(handle_);
}

void OnDemandService::stopAndClear() const {
    isolated::ondemand::IsolatedOnDemandService::stopAndClear(handle_);
}

void OnDemandService::setSpeed(double speed) const {
    isolated::ondemand::IsolatedOnDemandService::setSpeed(handle_, speed);
}

DXFCPP_END_NAMESPACE