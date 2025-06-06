// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

struct EventClassList::Impl : public RawListWrapper<dxfg_event_clazz_list_t, [](dxfg_event_clazz_list_t &list,
                                                                                std::size_t index, std::uint32_t id) {
    *list.elements[index] = static_cast<dxfg_event_clazz_t>(id);
}> {};

EventClassList::EventClassList() noexcept : impl_(std::make_unique<EventClassList::Impl>()) {
}

std::unique_ptr<EventClassList> EventClassList::create(std::size_t size) {
    auto result = std::unique_ptr<EventClassList>(new EventClassList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

void EventClassList::set(std::size_t index, std::uint32_t id) noexcept {
    impl_->set(index, id);
}

bool EventClassList::isEmpty() const noexcept {
    return impl_->isEmpty();
}

std::size_t EventClassList::size() const noexcept {
    return impl_->size();
}

void *EventClassList::getHandle() noexcept {
    return impl_->getHandle();
}

EventClassList::~EventClassList() noexcept = default;

DXFCPP_END_NAMESPACE