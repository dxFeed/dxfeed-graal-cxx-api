// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDXPublisherObservableSubscription.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisherObservableSubscription {

bool isClosed(const JavaObjectHandle<DXPublisherObservableSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_isClosed`. The `sub` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_ObservableSubscription_isClosed,
                                        static_cast<dxfg_v4_observable_subscription_t *>(sub.get())) == 1;
}

std::unordered_set<EventTypeEnum> getEventTypes(const JavaObjectHandle<DXPublisherObservableSubscription> &sub) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_getEventTypes`. The `sub` handle is invalid");
    }

    return event::IsolatedEventClazzList::takeEventTypes(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_ObservableSubscription_getEventTypes,
                                            static_cast<dxfg_v4_observable_subscription_t *>(sub.get())),
        "DXPublisherObservableSubscription");
}

bool containsEventType(const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
                       const EventTypeEnum &eventType) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_containsEventType`. The `sub` handle is "
            "invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_ObservableSubscription_containsEventType,
                                        static_cast<dxfg_v4_observable_subscription_t *>(sub.get()),
                                        static_cast<dxfg_v4_event_clazz_t>(eventType.getId())) == 1;
}

void addChangeListener(const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
                       const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener) {
    if (!sub) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_addChangeListener`. The `sub` handle is "
            "invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_addChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_ObservableSubscription_addChangeListener,
                       static_cast<dxfg_v4_observable_subscription_t *>(sub.get()),
                       static_cast<dxfg_v4_observable_subscription_change_listener_t *>(listener.get()));
}

void removeChangeListener(const JavaObjectHandle<DXPublisherObservableSubscription> &sub,
                          const JavaObjectHandle<ObservableSubscriptionChangeListener> &listener) {
    if (!sub) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_ObservableSubscription_removeChangeListener`. "
                                       "The `sub` handle is invalid");
    }

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ObservableSubscription_removeChangeListener`. The "
            "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_ObservableSubscription_removeChangeListener,
                       static_cast<dxfg_v4_observable_subscription_t *>(sub.get()),
                       static_cast<dxfg_v4_observable_subscription_change_listener_t *>(listener.get()));
}
} // namespace isolated::api::IsolatedDXPublisherObservableSubscription

DXFCPP_END_NAMESPACE