// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/EventClassList.hpp"
#include "../internal/context/ApiContext.hpp"

#include "../entity/EntityModule.hpp"
#include "osub/ObservableSubscription.hpp"
#include "../event/EventType.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../symbols/SymbolWrapper.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

class DXFCPP_EXPORT DXPublisherObservableSubscription : public ObservableSubscription, public SharedEntity {
    JavaObjectHandle<DXPublisherObservableSubscription> handle_;

  public:
    bool isClosed() override;
    std::unordered_set<EventTypeEnum> getEventTypes() override;
    bool containsEventType(const EventTypeEnum &eventType) override;
    std::size_t addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) override;
    void removeChangeListener(std::size_t id) override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()