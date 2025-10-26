// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

// #include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

#include "dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp"
#include "dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "dxfeed_graal_cpp_api/internal/managers/EntityManager.hpp"
#include "dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeedSubscription.hpp"

#include <fmt/format.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct DXFeedSubscription::Impl {
    static void onEvents(graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
        const auto id = Id<DXFeedSubscription>::from(dxfcpp::bit_cast<Id<DXFeedSubscription>::ValueType>(userData));

        if (auto sub = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>()->getEntity(id)) {
            auto &&events = EventMapper::fromGraalList(static_cast<void *>(graalNativeEvents));

            sub->onEvent_(events);
        }
    }
};

JavaObjectHandle<DXFeedSubscription>
DXFeedSubscription::createSubscriptionHandleFromEventClassList(const std::unique_ptr<EventClassList> &list) {
    return isolated::api::IsolatedDXFeedSubscription::create(list);
}

void DXFeedSubscription::setEventListenerHandle(Id<DXFeedSubscription> id) {
    eventListenerHandle_ = isolated::api::IsolatedDXFeedSubscription::DXFeedEventListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onEvents), dxfcpp::bit_cast<void *>(id.getValue()));

    isolated::api::IsolatedDXFeedSubscription::addEventListener(handle_, eventListenerHandle_);
}

bool DXFeedSubscription::tryToSetEventListenerHandle() {
    std::lock_guard lock{eventListenerMutex_};

    if (!eventListenerHandle_) {
        const auto idOpt = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>()->getId(
            sharedAs<DXFeedSubscription>());

        if (!idOpt) {
            return false;
        }

        setEventListenerHandle(idOpt.value());
    }

    return true;
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::setSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::addSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::removeSymbols(handle_, graalSymbolList);
}

DXFeedSubscription::DXFeedSubscription() : impl_(std::make_unique<Impl>()) {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", 1);
#endif
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) : DXFeedSubscription() {
    eventTypes_ = std::unordered_set{eventType};
    handle_ = isolated::api::IsolatedDXFeedSubscription::create(eventType);
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType, JavaObjectHandle<DXFeedSubscription> &&handle)
    : DXFeedSubscription() {
    eventTypes_ = std::unordered_set{eventType};
    handle_ = std::move(handle);
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag) : DXFeedSubscription() {
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag, const EventTypeEnum &eventType)
    : DXFeedSubscription(eventType) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }
}

std::string DXFeedSubscription::toString() const {
    return fmt::format("DXFeedSubscription{{{}}}", handle_.toString());
}

DXFeedSubscription::~DXFeedSubscription() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription{" + handle_.toString() + "}::~DXFeedSubscription()");
    }

    if (handle_) {
        close();
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXFeedSubscription", -1);
#endif
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(const EventTypeEnum &eventType) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription::create(eventType = " + eventType.getName() + ")");
    }

    auto sub = createShared(eventType);
    const auto id = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>()->registerEntity(sub);

    ignoreUnused(id);

    return sub;
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(std::initializer_list<EventTypeEnum> eventTypes) {
    auto sub = createShared(eventTypes);
    const auto id = ApiContext::getInstance()->getManager<EntityManager<DXFeedSubscription>>()->registerEntity(sub);

    ignoreUnused(id);

    return sub;
}

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::attach(feed = " + feed->toString() + ")");
    }

    feed->attachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::detach(feed = " + feed->toString() + ")");
    }

    feed->detachSubscription(sharedAs<DXFeedSubscription>());
}

bool DXFeedSubscription::isClosed() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::isClosed()");
    }

    return isolated::api::IsolatedDXFeedSubscription::isClosed(handle_);
}

void DXFeedSubscription::close() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::close()");
    }

    isolated::api::IsolatedDXFeedSubscription::close(handle_);
}

std::unordered_set<EventTypeEnum> DXFeedSubscription::getEventTypes() {
    return eventTypes_;
}

bool DXFeedSubscription::containsEventType(const EventTypeEnum &eventType) {
    return eventTypes_.contains(eventType);
}

void DXFeedSubscription::clear() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::clear()");
    }

    isolated::api::IsolatedDXFeedSubscription::clear(handle_);
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbols() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::getSymbols()");
    }

    return isolated::api::IsolatedDXFeedSubscription::getSymbols(handle_);
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbols() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::getDecoratedSymbols()");
    }

    return isolated::api::IsolatedDXFeedSubscription::getDecoratedSymbols(handle_);
}

void DXFeedSubscription::setSymbols(std::initializer_list<SymbolWrapper> collection) const {
    setSymbols(collection.begin(), collection.end());
}

void DXFeedSubscription::addSymbols(const SymbolWrapper &symbolWrapper) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::addSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
    }

    const auto graal = symbolWrapper.toGraalUnique();

    isolated::api::IsolatedDXFeedSubscription::addSymbol(handle_, graal.get());
}

void DXFeedSubscription::addSymbols(std::initializer_list<SymbolWrapper> collection) const {
    addSymbols(collection.begin(), collection.end());
}

void DXFeedSubscription::removeSymbols(const SymbolWrapper &symbolWrapper) const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::removeSymbols(symbolWrapper = " + toStringAny(symbolWrapper) + ")");
    }

    const auto graal = symbolWrapper.toGraalUnique();

    isolated::api::IsolatedDXFeedSubscription::removeSymbol(handle_, graal.get());
}

void DXFeedSubscription::removeSymbols(std::initializer_list<SymbolWrapper> collection) const {
    removeSymbols(collection.begin(), collection.end());
}

TimePeriod DXFeedSubscription::getAggregationPeriod() const {
    return TimePeriod(isolated::api::IsolatedDXFeedSubscription::getAggregationPeriod(handle_));
}

void DXFeedSubscription::setAggregationPeriod(const TimePeriod &aggregationPeriod) const {
    isolated::api::IsolatedDXFeedSubscription::setAggregationPeriod(handle_, aggregationPeriod.handle_);
}

void DXFeedSubscription::removeEventListener(std::size_t listenerId) {
    onEvent_ -= listenerId;
}

DXFeedSubscription::OnEventHandler &DXFeedSubscription::onEvent() {
    tryToSetEventListenerHandle();

    return onEvent_;
}

std::size_t DXFeedSubscription::addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) {
    isolated::api::IsolatedDXFeedSubscription::addChangeListener(
        handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

    std::lock_guard guard{changeListenersMutex_};

    if (lastChangeListenerId_ >= FAKE_CHANGE_LISTENER_ID - 1) {
        return FAKE_CHANGE_LISTENER_ID;
    }

    auto id = ++lastChangeListenerId_;

    changeListeners_.emplace(id, listener);

    return id;
}

void DXFeedSubscription::removeChangeListener(std::size_t changeListenerId) {
    std::lock_guard guard{changeListenersMutex_};

    if (changeListenerId == FAKE_CHANGE_LISTENER_ID) {
        return;
    }

    if (auto found = changeListeners_.find(changeListenerId); found != changeListeners_.end()) {
        auto listener = found->second;

        isolated::api::IsolatedDXFeedSubscription::removeChangeListener(
            handle_, listener->getHandle(ObservableSubscriptionChangeListener::Key{}));

        changeListeners_.erase(found);
    }
}

std::int32_t DXFeedSubscription::getEventsBatchLimit() const {
    return isolated::api::IsolatedDXFeedSubscription::getEventsBatchLimit(handle_);
}

void DXFeedSubscription::setEventsBatchLimit(std::int32_t eventsBatchLimit) const {
    isolated::api::IsolatedDXFeedSubscription::setEventsBatchLimit(handle_, eventsBatchLimit);
}

DXFeedTimeSeriesSubscription::DXFeedTimeSeriesSubscription(
    RequireMakeShared<DXFeedTimeSeriesSubscription>::LockExternalConstructionTag) {

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFeedTimeSeriesSubscription::DXFeedTimeSeriesSubscription(
    RequireMakeShared<DXFeedTimeSeriesSubscription>::LockExternalConstructionTag, const EventTypeEnum &eventType,
    JavaObjectHandle<DXFeedSubscription> &&handle)
    : DXFeedSubscription(eventType, std::move(handle)) {

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

std::string DXFeedTimeSeriesSubscription::toString() const {
    return fmt::format("DXFeedTimeSeriesSubscription{{{}}}", handle_.toString());
}

std::int64_t DXFeedTimeSeriesSubscription::getFromTime() {
    return fromTime_;
}

void DXFeedTimeSeriesSubscription::setFromTime(std::int64_t fromTime) {
    if (fromTime != fromTime_) {
        fromTime_ = fromTime;
        isolated::api::IsolatedDXFeedTimeSeriesSubscription::setFromTime(handle_, fromTime);
    }
}

void DXFeedTimeSeriesSubscription::setFromTime(std::chrono::milliseconds fromTime) {
    setFromTime(fromTime.count());
}

DXFeedTimeSeriesSubscription::~DXFeedTimeSeriesSubscription() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", -1);
#endif
}

void DXFeedTimeSeriesSubscription::registerEntity() {
#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add(
        "Entity.DXFeedSubscription.DXFeedTimeSeriesSubscription", 1);
#endif
}

DXFCPP_END_NAMESPACE