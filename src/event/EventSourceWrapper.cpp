// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/event/EventSourceWrapper.hpp>

DXFCPP_BEGIN_NAMESPACE

std::ptrdiff_t EventSourceWrapper::ListUtils::calculateGraalListSize(std::ptrdiff_t initSize) noexcept {
    return GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::calculateSize(initSize);
}

void *EventSourceWrapper::ListUtils::newGraalList(std::ptrdiff_t size) {
    return GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::newList(size);
}

bool EventSourceWrapper::ListUtils::setGraalListElement(void *graalList, std::ptrdiff_t elementIdx,
                                                                   void *element) noexcept {
    return GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::setElement(graalList, elementIdx,
                                                                                          element);
}

bool EventSourceWrapper::ListUtils::freeGraalListElements(void *graalList, std::ptrdiff_t count) {
    return GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::freeElements(graalList, count);
}

void EventSourceWrapper::ListUtils::freeGraalList(void *graalList) {
    GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::freeList(graalList);
}

std::vector<EventSourceWrapper> EventSourceWrapper::ListUtils::fromGraalList(void *graalList) {
    return GraalListUtils<dxfg_indexed_event_source_list, EventSourceWrapper>::fromList(graalList);
}

template void *EventSourceWrapper::ListUtils::toGraalList<dxfcpp::EventSourceWrapper const *>(
    dxfcpp::EventSourceWrapper const *, dxfcpp::EventSourceWrapper const *);

EventSourceWrapper EventSourceWrapper::fromGraal(void *graalNative) {
    switch (static_cast<dxfg_indexed_event_source_t *>(graalNative)->type) {
    case ORDER_SOURCE:
        return OrderSource::valueOf(static_cast<dxfg_indexed_event_source_t *>(graalNative)->id);
    default:
        return IndexedEventSource::fromGraal(graalNative);
    }
}

DXFCPP_END_NAMESPACE