// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {
namespace IsolatedEventType {

// dxfg_EventType_new
/* dxfg_event_type_t* */ void *create(const StringLikeWrapper &symbolName,
                                      /* dxfg_event_clazz_t */ const EventTypeEnum &eventType) {
    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_EventType_new, symbolName.c_str(), static_cast<dxfg_event_clazz_t>(eventType.getId())));
}

// dxfg_EventType_release
/* int32_t */ void release(/* dxfg_event_type_t* */ void *eventType) {
    if (!eventType) {
        throw InvalidArgumentException("Unable to execute function `dxfg_EventType_release`. The `eventType` is null");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_EventType_release, dxfcpp::bit_cast<dxfg_event_type_t *>(eventType));
}

std::unique_ptr<void, decltype(&release)> toUnique(void *eventType) {
    return {eventType, release};
}

} // namespace IsolatedEventType

namespace IsolatedEventTypeList {
// dxfg_CList_EventType_release
/* int32_t */ void release(/* dxfg_event_type_list* */ void *eventTypes) {
    if (!eventTypes) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_CList_EventType_release`. The `eventTypes` is null");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_CList_EventType_release,
                                       dxfcpp::bit_cast<dxfg_event_type_list *>(eventTypes));
}

std::unique_ptr<void, decltype(&release)> toUnique(void *eventTypes) {
    return {eventTypes, release};
}
} // namespace IsolatedEventTypeList
} // namespace isolated::internal

DXFCPP_END_NAMESPACE