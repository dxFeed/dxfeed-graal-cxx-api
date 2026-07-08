// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../include/dxfeed_graal_c_api/api.h"
#include "../include/dxfeed_graal_cpp_api/api.hpp"
#include "../include/dxfeed_graal_cpp_api/entity/SharedEntity.hpp"
#include "../include/dxfeed_graal_cpp_api/event/EventType.hpp"

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

ApiContext::ApiContext() noexcept {
}

ApiContext::~ApiContext() noexcept {
}

std::shared_ptr<ApiContext> ApiContext::getInstance() noexcept {
    static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new ApiContext{});

    return instance;
}

auto C = ApiContext::getInstance();
auto MM = ApiContext::getInstance()->getManager<MetricsManager>();

SharedEntity::SharedEntity() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", 1);
#endif
}

SharedEntity::~SharedEntity() noexcept {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity", -1);
#endif
}

EventType::EventType() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", 1);
#endif
}

EventType::~EventType() noexcept {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.Event", -1);
#endif
}

std::int64_t EventType::getEventTime() const noexcept {
    return 0;
}

void EventType::setEventTime(std::int64_t eventTime) noexcept {
    ignoreUnused(eventTime);
    // The default implementation is empty
}

void EventType::assign(std::shared_ptr<EventType> event) {
    ignoreUnused(event);
}

std::string EventType::toString() const {
    return "EventType{}";
}

Error::Error(Error &&) noexcept = default;

Error &Error::operator=(Error &&) noexcept = default;

Error::~Error() noexcept {
}

Error::Error() noexcept {
}

Error::Error(std::size_t causeId, std::size_t groupId, std::string location, std::string message) noexcept
    : causeId{causeId}, threadId{std::this_thread::get_id()}, groupId{groupId}, location{std::move(location)},
      message{std::move(message)} {
}

ErrorHandlingManager::ErrorHandlingManager() noexcept {
}

ErrorHandlingManager::~ErrorHandlingManager() noexcept {
}

std::shared_ptr<ErrorHandlingManager> ErrorHandlingManager::getInstance() {
    static std::shared_ptr<ErrorHandlingManager> instance{new ErrorHandlingManager};

    return instance;
}

const Error &ErrorHandlingManager::registerError(Error error) noexcept {
    std::lock_guard lock{errorCollectionMutex_};

    const std::size_t id = nextId++;
    error.id = id;
    errorCollection_[id] = std::move(error);

    if (errorCollection_.size() > maxErrorCollectionCapacity_) {
        errorCollection_.erase(id - maxErrorCollectionCapacity_);
    }

    return errorCollection_[id];
}

const Error &ErrorHandlingManager::getLastError() noexcept {
    return errorCollection_[nextId - 1];
}

DXFCPP_END_NAMESPACE
