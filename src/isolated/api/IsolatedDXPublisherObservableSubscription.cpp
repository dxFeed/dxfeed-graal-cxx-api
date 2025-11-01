// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXPublisherObservableSubscription.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisherObservableSubscription {

bool /* int32_t */
isClosed(/* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_isClosed`. The `sub` handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_ObservableSubscription_isClosed,
                                                  static_cast<dxfg_observable_subscription_t *>(sub.get())) == 1;
}

std::unordered_set<EventTypeEnum> /* dxfg_event_clazz_list_t* */ getEventTypes(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_getEventTypes`. The `sub` handle is invalid");
    }

    std::unordered_set<EventTypeEnum> result{};

    dxfg_event_clazz_list_t *eventTypesList = runGraalFunctionAndThrowIfNullptr(
        dxfg_ObservableSubscription_getEventTypes, static_cast<dxfg_observable_subscription_t *>(sub.get()));

    if (eventTypesList->size > 0) {
        for (auto i = 0; i < eventTypesList->size; i++) {
            dxfg_event_clazz_t ec = *eventTypesList->elements[static_cast<std::size_t>(i)];

            if (auto etIt = EventTypeEnum::ALL_BY_ID.find(static_cast<std::uint32_t>(ec));
                etIt != EventTypeEnum::ALL_BY_ID.end()) {
                result.emplace(etIt->second.get());
            }
        }
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_EventClazz_release, eventTypesList);

    return result;
}

bool /* int32_t */ containsEventType(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_event_clazz_t */ const EventTypeEnum &eventType) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_containsEventType`. The `sub` handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_ObservableSubscription_containsEventType,
                                                  static_cast<dxfg_observable_subscription_t *>(sub.get()),
                                                  static_cast<dxfg_event_clazz_t>(eventType.getId())) == 1;
}

void /* int32_t */ addChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_addChangeListener`. The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_addChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_ObservableSubscription_addChangeListener, static_cast<dxfg_observable_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}

void /* int32_t */ removeChangeListener(
    /* dxfg_observable_subscription_t * */ const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
    /* dxfg_observable_subscription_change_listener_t * */ const JavaObjectHandle<ObservableSubscriptionChangeListener>
        &listener) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_ObservableSubscription_removeChangeListener`. "
                                       "The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ObservableSubscription_removeChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_ObservableSubscription_removeChangeListener, static_cast<dxfg_observable_subscription_t *>(sub.get()),
        static_cast<dxfg_observable_subscription_change_listener_t *>(listener.get()));
}
} // namespace isolated::api::IsolatedDXPublisherObservableSubscription

DXFCPP_END_NAMESPACE