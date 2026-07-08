// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "../../include/dxfeed_graal_cpp_api/event/EventSourceWrapper.hpp"

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

EventSourceWrapper::EventSourceWrapper(const EventSourceWrapper &) noexcept = default;
EventSourceWrapper::EventSourceWrapper(EventSourceWrapper &&) noexcept = default;
EventSourceWrapper &EventSourceWrapper::operator=(const EventSourceWrapper &) noexcept = default;
EventSourceWrapper &EventSourceWrapper::operator=(EventSourceWrapper &&) noexcept = default;

EventSourceWrapper::EventSourceWrapper() noexcept {
}

EventSourceWrapper::~EventSourceWrapper() noexcept {
}

EventSourceWrapper::EventSourceWrapper(const IndexedEventSource &data) noexcept { // NOLINT(*-explicit-constructor)
    data_ = data;
}

EventSourceWrapper::EventSourceWrapper(const OrderSource &data) noexcept { // NOLINT(*-explicit-constructor)
    data_ = data;
}

void EventSourceWrapper::freeGraal(void *graalNative) {
    IndexedEventSource::freeGraal(graalNative);
}

template void *EventSourceWrapper::ListUtils::toGraalList<EventSourceWrapper const *>(EventSourceWrapper const *,
                                                                                      EventSourceWrapper const *);

EventSourceWrapper EventSourceWrapper::fromGraal(void *graalNative) {
    switch (static_cast<dxfg_indexed_event_source_t *>(graalNative)->type) {
    case ORDER_SOURCE:
        return OrderSource::valueOf(static_cast<dxfg_indexed_event_source_t *>(graalNative)->id);
    default:
        return IndexedEventSource::fromGraal(graalNative);
    }
}

void *EventSourceWrapper::toGraal() const noexcept {
    return std::visit(
        [](const auto &eventSource) {
            return eventSource.toGraal();
        },
        data_);
}

std::unique_ptr<void, decltype(&EventSourceWrapper::freeGraal)>
EventSourceWrapper::toGraalUnique() const noexcept { // NOLINT(*-use-nodiscard)
    return {toGraal(), freeGraal};
}

std::string EventSourceWrapper::toString() const {
    return "EventSourceWrapper{" +
           std::visit(
               [](const auto &eventSource) {
                   return toStringAny(eventSource);
               },
               data_) +
           "}";
}

std::string EventSourceWrapper::toStringUnderlying() const {
    return std::visit(
        [](const auto &eventSource) {
            return toStringAny(eventSource);
        },
        data_);
}

bool EventSourceWrapper::isIndexedEventSource() const noexcept {
    return std::holds_alternative<IndexedEventSource>(data_);
}

bool EventSourceWrapper::isOrderSource() const noexcept {
    return std::holds_alternative<OrderSource>(data_);
}

std::optional<IndexedEventSource> EventSourceWrapper::asIndexedEventSource() const noexcept {
    return isIndexedEventSource() ? std::make_optional<IndexedEventSource>(std::get<IndexedEventSource>(data_))
                                  : std::nullopt;
}

std::optional<OrderSource> EventSourceWrapper::asOrderSource() const noexcept {
    return isOrderSource() ? std::make_optional<OrderSource>(std::get<OrderSource>(data_)) : std::nullopt;
}

const EventSourceWrapper::DataType &EventSourceWrapper::getData() const noexcept {
    return data_;
}

DXFCPP_END_NAMESPACE