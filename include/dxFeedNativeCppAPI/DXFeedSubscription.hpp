// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_subscription.h>

#include <memory>

namespace dxfcpp {

template <typename Collection>
concept ElementIsEventTypeEnum =
    requires(Collection &&c) { std::is_same_v<std::decay_t<decltype(*std::begin(c))>, EventTypeEnum>; };

struct DXFeedSubscription : std::enable_shared_from_this<DXFeedSubscription> {
    mutable std::recursive_mutex mtx_{};
    dxfg_subscription_t *handle_ = nullptr;

    virtual ~DXFeedSubscription() = default;

    explicit DXFeedSubscription(EventTypeEnum eventType) noexcept : mtx_{}, handle_{nullptr} {
        handle_ = detail::Isolate::getInstance()->runIsolatedOrElse(
            [eventType](auto threadHandle) {
                return dxfg_DXFeedSubscription_new(threadHandle, eventType.getDxFeedGraalNativeApiEventClazz());
            },
            nullptr);
    }

    template <typename EventTypeIt>
    DXFeedSubscription(EventTypeIt begin, EventTypeIt end) noexcept : mtx_{}, handle_{nullptr} {
        dxfg_event_clazz_list_t l{0, nullptr};

        auto clazzListCleaner = detail::finally([&l] {
            if (l.size == 0 || l.elements == nullptr) {
                return;
            }

            for (auto i = l.size - 1; i >= 0; i--) {
                delete l.elements[i];
            }

            delete[] l.elements;
        });

        auto size = std::distance(begin, end);

        if (size <= 0) {
            return;
        }

        l.size = static_cast<std::int32_t>(size);
        l.elements = new (std::nothrow) dxfg_event_clazz_t *[size];

        if (!l.elements) {
            return;
        }

        std::size_t i = 0;

        for (auto it = begin; it != end; it++, i++) {
            l.elements[i] = new (std::nothrow) dxfg_event_clazz_t{it->getDxFeedGraalNativeApiEventClazz()};
        }

        handle_ = detail::Isolate::getInstance()->runIsolatedOrElse(
            [&l](auto threadHandle) { return dxfg_DXFeedSubscription_new2(threadHandle, &l); }, nullptr);
    }

    DXFeedSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept
        : DXFeedSubscription(eventTypes.begin(), eventTypes.end()) {}

    template <typename EventTypesCollection>
    explicit DXFeedSubscription(EventTypesCollection &&eventTypes) noexcept
        requires requires { ElementIsEventTypeEnum<EventTypesCollection>; }
        : DXFeedSubscription(std::begin(std::forward<EventTypesCollection>(eventTypes)),
                             std::end(std::forward<EventTypesCollection>(eventTypes))) {}
};

} // namespace dxfcpp