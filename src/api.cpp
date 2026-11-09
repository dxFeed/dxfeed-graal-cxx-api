// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../include/dxfeed_graal_c_api/api.h"
#include "../include/dxfeed_graal_cpp_api/api.hpp"
#include "../include/dxfeed_graal_cpp_api/entity/SharedEntity.hpp"
#include "../include/dxfeed_graal_cpp_api/event/EventType.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

ApiContext::ApiContext() noexcept {
}

ApiContext::~ApiContext() noexcept {
}

std::shared_ptr<ApiContext> ApiContext::getInstance() noexcept {
    static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new ApiContext{});

    return instance;
}

std::shared_ptr<void> ApiContext::getManagerImpl(const char *typeName,
                                                 std::shared_ptr<void> (*create)()) const noexcept {
    // The registry does not own the managers: the statics of getManager() in the modules do, so a manager lives as long
    // as it did before the registry (it is destroyed with the statics of the module that created it first, before the
    // isolate; owning it here kept the endpoints of the managers alive until the end of the exit, and closing them then
    // hung the process).
    static std::mutex mutex{};
    static std::unordered_map<std::string, std::weak_ptr<void>> managers{};

    std::lock_guard lock(mutex);
    auto &registered = managers[typeName];
    auto manager = registered.lock();

    if (!manager) {
        manager = create();
        registered = manager;
    }

    return manager;
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

Error::Error(std::size_t errorCauseId, std::size_t errorGroupId, std::string errorLocation,
             std::string errorMessage) noexcept
    : causeId{errorCauseId}, threadId{std::this_thread::get_id()}, groupId{errorGroupId},
      location{std::move(errorLocation)}, message{std::move(errorMessage)} {
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
