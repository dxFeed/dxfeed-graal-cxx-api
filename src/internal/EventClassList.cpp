// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/EventClassList.hpp"

#include "../../include/dxfeed_graal_cpp_api/internal/RawListWrapper.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

struct EventClassList::Impl : RawListWrapper<dxfg_event_clazz_list_t, [](dxfg_event_clazz_list_t &list,
                                                                                std::size_t index, std::uint32_t id) {
    *list.elements[index] = static_cast<dxfg_event_clazz_t>(id);
}> {};

void EventClassList::set(std::size_t index, std::uint32_t id) const noexcept {
    impl_->set(index, id);
}

bool EventClassList::isEmpty() const noexcept {
    return impl_->isEmpty();
}

std::size_t EventClassList::size() const noexcept {
    return impl_->size();
}

void *EventClassList::getHandle() const noexcept {
    return impl_->getHandle();
}

EventClassList::~EventClassList() noexcept = default;

std::unique_ptr<EventClassList> EventClassList::create(std::size_t size) {
    auto result = std::unique_ptr<EventClassList>(new EventClassList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

EventClassList::EventClassList() noexcept : impl_(std::make_unique<Impl>()) {
}

DXFCPP_END_NAMESPACE