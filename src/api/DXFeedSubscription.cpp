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

namespace dxfcpp {

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::attach(feed = " + feed->toString() + ")");
    }

    feed->attachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::detach(feed = " + feed->toString() + ")");
    }

    feed->detachSubscription(sharedAs<DXFeedSubscription>());
}

void DXFeedSubscription::addSymbolImpl(void *graalSymbol) const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_addSymbol(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
                   0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()), static_cast<dxfg_symbol_t *>(graalSymbol));
}

void DXFeedSubscription::addSymbolsImpl(void *graalSymbolList) const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_addSymbols(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
                   0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()), static_cast<dxfg_symbol_list *>(graalSymbolList));
}

void DXFeedSubscription::removeSymbolImpl(void *graalSymbol) const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_removeSymbol(static_cast<graal_isolatethread_t *>(threadHandle),
                                                        params...) == 0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()), static_cast<dxfg_symbol_t *>(graalSymbol));
}

void DXFeedSubscription::removeSymbolsImpl(void *graalSymbolList) const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_removeSymbols(static_cast<graal_isolatethread_t *>(threadHandle),
                                                         params...) == 0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()), static_cast<dxfg_symbol_list *>(graalSymbolList));
}

void DXFeedSubscription::setSymbolsImpl(void *graalSymbolList) const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_setSymbols(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
                   0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()), static_cast<dxfg_symbol_list *>(graalSymbolList));
}

std::vector<SymbolWrapper> DXFeedSubscription::getSymbolsImpl() const noexcept {
    if (!handle_) {
        return {};
    }

    dxfg_symbol_list *list = runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_getSymbols(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        nullptr, static_cast<dxfg_subscription_t *>(handle_.get()));

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_CList_symbol_release(static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, list);

    return result;
}

std::vector<SymbolWrapper> DXFeedSubscription::getDecoratedSymbolsImpl() const noexcept {
    if (!handle_) {
        return {};
    }

    dxfg_symbol_list *list = runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_getDecoratedSymbols(static_cast<graal_isolatethread_t *>(threadHandle),
                                                               params...);
        },
        nullptr, static_cast<dxfg_subscription_t *>(handle_.get()));

    auto result = SymbolWrapper::SymbolListUtils::fromGraalList(static_cast<void *>(list));

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_CList_symbol_release(static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, list);

    return result;
}

void DXFeedSubscription::closeImpl() const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_close(static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()));
}

void DXFeedSubscription::clearImpl() const noexcept {
    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_clear(static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()));
}

bool DXFeedSubscription::isClosedImpl() const noexcept {
    if (!handle_) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_isClosed(static_cast<graal_isolatethread_t *>(threadHandle), params...) != 0;
        },
        false, static_cast<dxfg_subscription_t *>(handle_.get()));
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) noexcept
    : eventTypes_{eventType}, handle_{}, eventListenerHandle_{}, onEvent_{} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXFeedSubscription(eventType = " + eventType.getName() + ")");
    }

    handle_ = JavaObjectHandle<DXFeedSubscription>(runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_new(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        nullptr, static_cast<dxfg_event_clazz_t>(eventType.getId())));
}

JavaObjectHandle<DXFeedSubscription>
DXFeedSubscription::createSubscriptionHandleFromEventClassList(const std::unique_ptr<EventClassList> &list) noexcept {
    return JavaObjectHandle<DXFeedSubscription>(runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedSubscription_new2(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        nullptr, static_cast<dxfg_event_clazz_list_t *>(list->getHandle())));
}

void DXFeedSubscription::setEventListenerHandle(Id<DXFeedSubscription> id) noexcept {
    // TODO: move to Impl!
    auto onEvents = [](graal_isolatethread_t * /*thread*/, dxfg_event_type_list *graalNativeEvents, void *userData) {
        auto id = Id<DXFeedSubscription>::from(dxfcpp::bit_cast<Id<DXFeedSubscription>::ValueType>(userData));
        auto sub = ApiContext::getInstance()->getManager<DXFeedSubscriptionManager>()->getEntity(id);

        if (sub) {
            auto &&events = EventMapper::fromGraalList(static_cast<void *>(graalNativeEvents));

            sub->onEvent_(events);
        }
    };

    eventListenerHandle_ = JavaObjectHandle<DXFeedEventListener>(runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_DXFeedEventListener_new(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        nullptr, onEvents, dxfcpp::bit_cast<void *>(id.getValue())));

    if (handle_ && eventListenerHandle_) {
        runIsolatedOrElse(
            [](auto threadHandle, auto &&...params) {
                return dxfg_DXFeedSubscription_addEventListener(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                params...) == 0;
            },
            false, static_cast<dxfg_subscription_t *>(handle_.get()),
            static_cast<dxfg_feed_event_listener_t *>(eventListenerHandle_.get()));
    }
}

bool DXFeedSubscription::tryToSetEventListenerHandle() noexcept {
    std::lock_guard lock{listenerMutex_};

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

} // namespace dxfcpp