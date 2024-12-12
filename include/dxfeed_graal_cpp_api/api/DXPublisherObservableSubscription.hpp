// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/EntityModule.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "osub/ObservableSubscription.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT DXPublisherObservableSubscription : RequireMakeShared<DXPublisherObservableSubscription>,
                                                         ObservableSubscription {
    static constexpr std::size_t FAKE_LISTENER_ID{static_cast<std::size_t>(-1)};

  private:
    inline static std::atomic<std::size_t> lastListenerId_{};

    JavaObjectHandle<DXPublisherObservableSubscription> handle_;
    std::unordered_map<std::size_t, std::shared_ptr<ObservableSubscriptionChangeListener>> listeners_;
    std::recursive_mutex listenersMutex_{};

  public:
    DXPublisherObservableSubscription(LockExternalConstructionTag,
                                      JavaObjectHandle<DXPublisherObservableSubscription> &&handle);
    ~DXPublisherObservableSubscription() override;

    static std::shared_ptr<DXPublisherObservableSubscription>
    create(JavaObjectHandle<DXPublisherObservableSubscription> &&handle);

    bool isClosed() override;
    std::unordered_set<EventTypeEnum> getEventTypes() override;
    bool containsEventType(const EventTypeEnum &eventType) override;
    std::size_t addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) override;
    void removeChangeListener(std::size_t changeListenerId) override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()