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

void DXFeedSubscription::close() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::close()");
    }

    closeImpl();
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

void DXFeedSubscription::addSymbolImpl(void *graalSymbol) const {
    isolated::api::IsolatedDXFeedSubscription::addSymbol(handle_, graalSymbol);
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::addSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::removeSymbolImpl(void *graalSymbol) const {
    isolated::api::IsolatedDXFeedSubscription::removeSymbol(handle_, graalSymbol);
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::removeSymbols(handle_, graalSymbolList);
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const {
    isolated::api::IsolatedDXFeedSubscription::setSymbols(handle_, graalSymbolList);
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbolsImpl() const {
    return isolated::api::IsolatedDXFeedSubscription::getSymbols(handle_);
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbolsImpl() const {
    return isolated::api::IsolatedDXFeedSubscription::getDecoratedSymbols(handle_);
}

void DXFeedSubscription::closeImpl() const {
    isolated::api::IsolatedDXFeedSubscription::close(handle_);
}

void DXFeedSubscription::clearImpl() const {
     isolated::api::IsolatedDXFeedSubscription::clear(handle_);
}

bool DXFeedSubscription::isClosedImpl() const {
    return isolated::api::IsolatedDXFeedSubscription::isClosed(handle_);
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

std::string DXFeedSubscription::toString() const noexcept {
    return fmt::format("DXFeedSubscription{{{}}}", handle_.toString());
}

DXFeedSubscription::~DXFeedSubscription() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXFeedSubscription{" + handle_.toString() + "}::~DXFeedSubscription()");
    }

    closeImpl();
}

DXFCPP_END_NAMESPACE