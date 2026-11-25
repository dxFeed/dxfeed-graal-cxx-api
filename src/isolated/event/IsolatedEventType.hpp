// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"


DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"
#include "../../internal/Common.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::event {

namespace IsolatedIndexedEventSource {

/// dxfg_v4_IndexedEventSource_release; `NULL` is skipped.
void release(void *source);

} // namespace IsolatedIndexedEventSource

namespace IsolatedIndexedEventSourceList {

/// dxfg_v4_indexed_event_source_list_release: releases the sources of the list with the list; `NULL` is skipped.
void release(/* dxfg_v4_indexed_event_source_list* */ void *sources);

} // namespace IsolatedIndexedEventSourceList

namespace IsolatedEventType {

/// dxfg_v4_EventType_new
/* dxfg_v4_event_type_t* */ void *create(std::string_view symbolName,
                                         /* dxfg_v4_event_clazz_t */ const EventTypeEnum &eventType);

/// dxfg_v4_EventType_release
/* int32_t */ void release(/* dxfg_v4_event_type_t* */ void *eventType);

std::unique_ptr<void, decltype(&release)> toUnique(void *eventType);

} // namespace IsolatedEventType

namespace IsolatedEventTypeList {

/// dxfg_v4_event_type_list_release
/* int32_t */ void release(/* dxfg_v4_event_type_list* */ void *eventTypes);

std::unique_ptr<void, decltype(&release)> toUnique(void *eventTypes);

} // namespace IsolatedEventTypeList

namespace IsolatedEventClazzList {

/**
 * Takes the event types of a list returned by the SDK and releases the list with dxfg_v4_event_clazz_list_release.
 * Unknown event type ids are skipped. A failure to release the list is reported by runRelease() on behalf of
 * `component`, it is not thrown.
 *
 * @param list The list (`dxfg_v4_event_clazz_list*`), not `NULL`.
 * @param component The name of the component that reports a failure to release the list.
 * @return The known event types of the list.
 */
std::unordered_set<EventTypeEnum> takeEventTypes(/* dxfg_v4_event_clazz_list* */ void *list, const char *component);

} // namespace IsolatedEventClazzList
} // namespace isolated::event

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
