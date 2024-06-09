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
    };
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
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get()), graalSymbol](auto threadHandle) {
            return dxfg_DXFeedSubscription_addSymbol(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                     static_cast<dxfg_symbol_t *>(graalSymbol)) == 0;
        },
        false);
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_addSymbols(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                      static_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

void DXFeedSubscription::removeSymbolImpl(void *graalSymbol) const {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get()), graalSymbol](auto threadHandle) {
            return dxfg_DXFeedSubscription_removeSymbol(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                        static_cast<dxfg_symbol_t *>(graalSymbol)) == 0;
        },
        false);
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_removeSymbols(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                         static_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get()), graalSymbolList](auto threadHandle) {
            return dxfg_DXFeedSubscription_setSymbols(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                      static_cast<dxfg_symbol_list *>(graalSymbolList)) == 0;
        },
        false);
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbolsImpl() const {
    if (!handle_) {
        return {};
    }

    dxfg_symbol_list *list = runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_getSymbols(static_cast<graal_isolatethread_t *>(threadHandle), handle);
        },
        nullptr);

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runIsolatedOrElse(
        [list](auto threadHandle) {
            return dxfg_CList_symbol_release(static_cast<graal_isolatethread_t *>(threadHandle), list) == 0;
        },
        false);

    return result;
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbolsImpl() const {
    if (!handle_) {
        return {};
    }

    dxfg_symbol_list *list = runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_getDecoratedSymbols(static_cast<graal_isolatethread_t *>(threadHandle),
                                                               handle);
        },
        nullptr);

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runIsolatedOrElse(
        [list](auto threadHandle) {
            return dxfg_CList_symbol_release(static_cast<graal_isolatethread_t *>(threadHandle), list) == 0;
        },
        false);

    return result;
}

void DXFeedSubscription::closeImpl() const {
    isolated::api::IsolatedDXFeedSubscription::close(handle_);
}

void DXFeedSubscription::clearImpl() const {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_clear(static_cast<graal_isolatethread_t *>(threadHandle), handle) == 0;
        },
        false);
}

bool DXFeedSubscription::isClosedImpl() const {
    if (!handle_) {
        return false;
    }

    return runIsolatedOrElse(
        [handle = static_cast<dxfg_subscription_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_isClosed(static_cast<graal_isolatethread_t *>(threadHandle), handle) != 0;
        },
        false);
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag)
    : impl_(std::make_unique<DXFeedSubscription::Impl>()) {
}

DXFeedSubscription::DXFeedSubscription(LockExternalConstructionTag tag, const EventTypeEnum &eventType)
    : DXFeedSubscription{tag} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }

    eventTypes_ = {eventType};
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