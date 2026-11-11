// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"


DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/event/EventTypeEnum.hpp"
#include "../../internal/Common.hpp"

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
} // namespace isolated::event

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
