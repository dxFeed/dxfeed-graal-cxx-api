// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <typeinfo>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

struct DXFeedSubscription::Impl {
    static void onEvents(graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
        auto id = Id<DXFeedSubscription>::from(dxfcpp::bit_cast<Id<DXFeedSubscription>::ValueType>(userData));
        auto sub = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->getEntity(id);

        if (sub) {
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
        auto idOpt =
            ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->getId(sharedAs<DXFeedSubscription>());

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

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag)
    : impl_(std::make_unique<DXFeedSubscription::Impl>()) {
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag tag, const EventTypeEnum &eventType)
    : DXFeedSubscription{tag} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }

    eventTypes_ = std::unordered_set{eventType};
    handle_ = isolated::api::IsolatedDXFeedSubscription::create(eventType);
}

std::string DXFeedSubscription::toString() const {
    return fmt::format("DXFeedSubscription{{{}}}", handle_.toString());
}

DXFeedSubscription::~DXFeedSubscription() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription{" + handle_.toString() + "}::~DXFeedSubscription()");
    }

    close();
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(const EventTypeEnum &eventType) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription::create(eventType = " + eventType.getName() + ")");
    }

    auto sub = createShared(eventType);
    auto id = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->registerEntity(sub);

    dxfcpp::ignoreUnused(id);

    return sub;
}

std::shared_ptr<DXFeedSubscription> DXFeedSubscription::create(std::initializer_list<EventTypeEnum> eventTypes) {
    auto sub = createShared(eventTypes);
    auto id = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->registerEntity(sub);

    dxfcpp::ignoreUnused(id);

    return sub;
}

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::attach(feed = " + feed->toString() + ")");
    }

    feed->attachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) {
    if constexpr (Debugger::isDebug) {
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

    auto graal = symbolWrapper.toGraalUnique();

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

    auto graal = symbolWrapper.toGraalUnique();

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
    isolated::api::IsolatedDXFeedSubscription::addChangeListener(handle_, listener->getHandle());

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

        isolated::api::IsolatedDXFeedSubscription::removeChangeListener(handle_, listener->getHandle());

        changeListeners_.erase(found);
    }
}

std::int32_t DXFeedSubscription::getEventsBatchLimit() const {
    return isolated::api::IsolatedDXFeedSubscription::getEventsBatchLimit(handle_);
}

void DXFeedSubscription::setEventsBatchLimit(std::int32_t eventsBatchLimit) const {
    isolated::api::IsolatedDXFeedSubscription::setEventsBatchLimit(handle_, eventsBatchLimit);
}

DXFCPP_END_NAMESPACE