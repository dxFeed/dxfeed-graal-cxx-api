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
    // Never deleted: the Graal callbacks may use the context until the process ends.
    static const auto &instance = *new std::shared_ptr<ApiContext>(new ApiContext{});

    return instance;
}

namespace {

/// The managers of the process by the name of their type.
struct ManagerRegistry {
    std::mutex mutex{};
    std::unordered_map<std::string, std::shared_ptr<void>> managers{};
};

} // namespace

std::shared_ptr<void> ApiContext::getManagerImpl(const char *typeName,
                                                 std::shared_ptr<void> (*create)()) const noexcept {
    // The registry owns the managers and is never deleted: the Graal callbacks look the entities up in the managers
    // until the process ends, and the entities are not destroyed at exit (their destructors would call Graal).
    static auto &registry = *new ManagerRegistry{};

    std::lock_guard lock(registry.mutex);
    auto &manager = registry.managers[typeName];

    if (!manager) {
        manager = create();
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

DXFCPP_END_NAMESPACE
