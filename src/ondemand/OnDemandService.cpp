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

std::shared_ptr<OnDemandService> OnDemandService::getInstance() noexcept {
    return getInstance(DXEndpoint::getInstance(DXEndpoint::Role::ON_DEMAND_FEED));
}

std::shared_ptr<OnDemandService> OnDemandService::getInstance(std::shared_ptr<DXEndpoint> endpoint) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("OnDemandService::getInstance(" + endpoint->toString() + ")");
    }

    std::shared_ptr<OnDemandService> onDemandService{new (std::nothrow) OnDemandService{}};

    if (!onDemandService || !endpoint->handle_) {
        // TODO: dummy service & error handling [EN-8232];

        return onDemandService;
    }

    auto id = ApiContext::getInstance()->getManager<OnDemandServiceManager>()->registerEntity(onDemandService);
    ignore_unused(id);

    onDemandService->endpoint_ = endpoint;
    onDemandService->handle_ =
        JavaObjectHandle<OnDemandService>(isolated::ondemand::OnDemandService::getInstance(endpoint->handle_.get()));

    return onDemandService;
}

std::shared_ptr<DXEndpoint> OnDemandService::getEndpoint() const noexcept {
    return endpoint_;
}

bool OnDemandService::isReplaySupported() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ondemand::OnDemandService::isReplaySupported(handle_.get());
}

bool OnDemandService::isReplay() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ondemand::OnDemandService::isReplay(handle_.get());
}

bool OnDemandService::isClear() const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ondemand::OnDemandService::isClear(handle_.get());
}

std::int64_t OnDemandService::getTime() const noexcept {
    if (!handle_) {
        return {};
    }

    return isolated::ondemand::OnDemandService::getTime(handle_.get());
}

double OnDemandService::getSpeed() const noexcept {
    if (!handle_) {
        return {};
    }

    return isolated::ondemand::OnDemandService::getSpeed(handle_.get());
}

void OnDemandService::replay(std::int64_t time) const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::replay(handle_.get(), time);
}

void OnDemandService::replay(std::int64_t time, double speed) const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::replay(handle_.get(), time, speed);
}

void OnDemandService::pause() const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::pause(handle_.get());
}

void OnDemandService::stopAndResume() const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::stopAndResume(handle_.get());
}

void OnDemandService::stopAndClear() const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::stopAndClear(handle_.get());
}

void OnDemandService::setSpeed(double speed) const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ondemand::OnDemandService::setSpeed(handle_.get(), speed);
}

DXFCPP_END_NAMESPACE