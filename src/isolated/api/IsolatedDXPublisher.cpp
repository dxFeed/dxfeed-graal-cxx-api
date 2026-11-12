// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDXPublisher.hpp"

#include "../../../include/dxfeed_graal_cpp_api/api/DXPublisherObservableSubscription.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXPublisher {

void publishEvents(const JavaObjectHandle<DXPublisher> &publisher, void *events) {
    if (!publisher) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXPublisher_publishEvents`. The `publisher` handle is invalid");
    }

    if (!events) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXPublisher_publishEvents`. The `events` is nullptr");
    }

    runGraalV4Function(dxfg_v4_DXPublisher_publishEvents, static_cast<dxfg_v4_publisher_t *>(publisher.get()),
                       static_cast<dxfg_v4_event_type_list *>(events));
}

JavaObjectHandle<DXPublisherObservableSubscription> getSubscription(const JavaObjectHandle<DXPublisher> &publisher,
                                                                     const EventTypeEnum &eventType) {
    if (!publisher) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXPublisher_getSubscription`. The `publisher` handle is invalid");
    }

    return JavaObjectHandle<DXPublisherObservableSubscription>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXPublisher_getSubscription, static_cast<dxfg_v4_publisher_t *>(publisher.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId())));
}

} // namespace isolated::api::IsolatedDXPublisher

DXFCPP_END_NAMESPACE