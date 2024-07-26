// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::event {
namespace IsolatedEventType {

// dxfg_EventType_new
/* dxfg_event_type_t* */ void *create(const StringLikeWrapper &symbolName,
                                      /* dxfg_event_clazz_t */ const EventTypeEnum &eventType);

// dxfg_EventType_release
/* int32_t */ void release(/* dxfg_event_type_t* */ void *eventType);

std::unique_ptr<void, decltype(&release)> toUnique(void *eventType);

} // namespace IsolatetEventType

namespace IsolatedEventTypeList {

// dxfg_CList_EventType_release
/* int32_t */ void release(/* dxfg_event_type_list* */ void *eventTypes);

std::unique_ptr<void, decltype(&release)> toUnique(void *eventTypes);

} // namespace IsolatetEventTypeList
} // namespace isolated::event

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()