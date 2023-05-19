// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>

namespace dxfcpp {

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (isDebug) {
        debug("{}::attach(feed = {})", toString(), feed->toString());
    }

    feed->attachSubscription(shared_from_this());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (isDebug) {
        debug("{}::detach(feed = {})", toString(), feed->toString());
    }

    feed->detachSubscription(shared_from_this());
}

template <> void DXFeedSubscription::addSymbol(std::string &&symbol) noexcept {
    runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get()), symbol](auto threadHandle) {
            dxfg_string_symbol_t s{{STRING}, symbol.c_str()};

            return dxfg_DXFeedSubscription_addSymbol(threadHandle, handler, (dxfg_symbol_t *)&s) == 0;
        },
        false);
}

void DXFeedSubscription::closeImpl() noexcept {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_subscription_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXFeedSubscription_close(threadHandle, handler) == 0; },
                      false);
}

void DXFeedSubscription::clearImpl() noexcept {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_subscription_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXFeedSubscription_clear(threadHandle, handler) == 0; },
                      false);
}

bool DXFeedSubscription::isClosedImpl() noexcept {
    if (!handler_) {
        return false;
    }

    return runIsolatedOrElse(
        [handler = bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_isClosed(threadHandle, handler) != 0;
        },
        false);
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) noexcept
    : mtx_{}, handler_{}, eventListenerHandler_{}, onEvent_{} {
    if constexpr (isDebug) {
        debug("DXFeedSubscription(eventType = {})", eventType.getName());
    }

    handler_ = handler_utils::JavaObjectHandler<DXFeedSubscription>(runIsolatedOrElse(
        [eventType](auto threadHandle) {
            return dxfg_DXFeedSubscription_new(threadHandle, static_cast<dxfg_event_clazz_t>(eventType.getId()));
        },
        nullptr));
}

handler_utils::JavaObjectHandler<DXFeedSubscription> DXFeedSubscription::createSubscriptionHandlerFromEventClassList(
    const std::unique_ptr<handler_utils::EventClassList> &list) noexcept {
    return handler_utils::JavaObjectHandler<DXFeedSubscription>(
        runIsolatedOrElse([listHandler = bit_cast<dxfg_event_clazz_list_t *>(list->getHandler())](
                              auto threadHandle) { return dxfg_DXFeedSubscription_new2(threadHandle, listHandler); },
                          nullptr));
}

void DXFeedSubscription::setEventListenerHandler(Id<DXFeedSubscription> id) noexcept {
    auto onEvents = [](graal_isolatethread_t *thread, dxfg_event_type_list *graalNativeEvents, void *userData) {
        auto id = Id<DXFeedSubscription>::from(bit_cast<Id<DXFeedSubscription>::ValueType>(userData));
        auto sub = ApiContext::getInstance()->getDxFeedSubscriptionManager()->getEntity(id);

        if (sub) {
            auto &&events = EventMapper::fromGraalNativeList(bit_cast<void *>(graalNativeEvents));

            sub->onEvent_(events);
        }
    };

    eventListenerHandler_ = handler_utils::JavaObjectHandler<DXFeedEventListener>(runIsolatedOrElse(
        [idValue = id.getValue(), onEvents](auto threadHandle) {
            return dxfg_DXFeedEventListener_new(threadHandle, onEvents, bit_cast<void *>(idValue));
        },
        nullptr));

    if (handler_ && eventListenerHandler_) {
        runIsolatedOrElse(
            [handler = bit_cast<dxfg_subscription_t *>(handler_.get()),
             eventListenerHandler =
                 bit_cast<dxfg_feed_event_listener_t *>(eventListenerHandler_.get())](auto threadHandle) {
                // TODO: finalize function

                return dxfg_DXFeedSubscription_addEventListener(
                           threadHandle, handler, eventListenerHandler, [](auto, auto) {}, nullptr) == 0;
            },
            false);
    }
}

} // namespace dxfcpp