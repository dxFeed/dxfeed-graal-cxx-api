// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXPublisher.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisher {

void /* int32_t */ publishEvents(/* dxfg_publisher_t * */ const JavaObjectHandle<dxfcpp::DXPublisher> &publisher,
                                 /* dxfg_event_type_list * */ void *events) {
    if (!publisher) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXPublisher_publishEvents`. The `publisher` handle is invalid");
    }

    if (!events) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXPublisher_publishEvents`. The `events` is nullptr");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXPublisher_publishEvents,
                                           static_cast<dxfg_publisher_t *>(publisher.get()),
                                           static_cast<dxfg_event_type_list *>(events));
}

JavaObjectHandle<DXPublisherObservableSubscription> /* dxfg_observable_subscription_t * */
getSubscription(/* dxfg_publisher_t * */ const JavaObjectHandle<dxfcpp::DXPublisher> &publisher,
                /* dxfg_event_clazz_t */ const EventTypeEnum &eventType) {
    if (!publisher) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXPublisher_getSubscription`. The `publisher` handle is invalid");
    }

    return JavaObjectHandle<DXPublisherObservableSubscription>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXPublisher_getSubscription, static_cast<dxfg_publisher_t *>(publisher.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId())));
}

} // namespace isolated::api::IsolatedDXPublisher

DXFCPP_END_NAMESPACE