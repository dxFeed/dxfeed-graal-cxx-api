// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedEventType.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/CallbackBarrier.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::event {

namespace IsolatedIndexedEventSource {

/// dxfg_v4_IndexedEventSource_release
void release(void *source) {
    if (source == nullptr) {
        return;
    }

    runGraalV4Function(dxfg_v4_IndexedEventSource_release, static_cast<dxfg_v4_indexed_event_source_t *>(source));
}

} // namespace IsolatedIndexedEventSource

namespace IsolatedIndexedEventSourceList {

/// dxfg_v4_indexed_event_source_list_release
void release(void *sources) {
    if (sources == nullptr) {
        return;
    }

    runGraalV4Function(dxfg_v4_indexed_event_source_list_release,
                       static_cast<dxfg_v4_indexed_event_source_list *>(sources));
}

} // namespace IsolatedIndexedEventSourceList

namespace IsolatedEventType {

/// dxfg_v4_EventType_new
/* dxfg_v4_event_type_t* */ void *create(std::string_view symbolName,
                                         /* dxfg_v4_event_clazz_t */ const EventTypeEnum &eventType) {
    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_EventType_new, SdkString{symbolName, "DXFeed::getLastEvent(event)"}.c_str(),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId())));
}

/// dxfg_v4_EventType_release
/* int32_t */ void release(/* dxfg_v4_event_type_t* */ void *eventType) {
    if (!eventType) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_EventType_release`. The `eventType` is null");
    }

    runGraalV4Function(dxfg_v4_EventType_release, dxfcpp::bit_cast<dxfg_v4_event_type_t *>(eventType));
}

std::unique_ptr<void, decltype(&release)> toUnique(void *eventType) {
    return {eventType, release};
}

} // namespace IsolatedEventType

namespace IsolatedEventTypeList {
/// dxfg_v4_event_type_list_release
/* int32_t */ void release(/* dxfg_v4_event_type_list* */ void *eventTypes) {
    if (!eventTypes) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_event_type_list_release`. The `eventTypes` is null");
    }

    runGraalV4Function(dxfg_v4_event_type_list_release, dxfcpp::bit_cast<dxfg_v4_event_type_list *>(eventTypes));
}

std::unique_ptr<void, decltype(&release)> toUnique(void *eventTypes) {
    return {eventTypes, release};
}
} // namespace IsolatedEventTypeList

namespace IsolatedEventClazzList {

std::unordered_set<EventTypeEnum> takeEventTypes(/* dxfg_v4_event_clazz_list* */ void *list, const char *component) {
    auto *eventTypesList = static_cast<dxfg_v4_event_clazz_list *>(list);
    // A failure to free the list must not replace the result or an exception of the conversion.
    const auto releaseList = [component](dxfg_v4_event_clazz_list *toRelease) noexcept {
        runRelease(component, [toRelease] {
            runGraalV4Function(dxfg_v4_event_clazz_list_release, toRelease);
        });
    };
    const std::unique_ptr<dxfg_v4_event_clazz_list, decltype(releaseList)> guard{eventTypesList, releaseList};
    std::unordered_set<EventTypeEnum> result{};

    for (std::int32_t i = 0; i < eventTypesList->size; i++) {
        const auto eventClazz = *eventTypesList->elements[static_cast<std::size_t>(i)];

        if (auto etIt = EventTypeEnum::ALL_BY_ID.find(static_cast<std::uint32_t>(eventClazz));
            etIt != EventTypeEnum::ALL_BY_ID.end()) {
            result.emplace(etIt->second.get());
        }
    }

    return result;
}

} // namespace IsolatedEventClazzList
} // namespace isolated::event

DXFCPP_END_NAMESPACE
