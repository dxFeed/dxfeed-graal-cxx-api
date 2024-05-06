// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../event/IndexedEventSource.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT ObservableSubscriptionChangeListener {
    virtual ~ObservableSubscriptionChangeListener() = default;

    virtual void symbolsAdded(const std::unordered_set<SymbolWrapper> &symbols) = 0;
    virtual void symbolsRemoved(const std::unordered_set<SymbolWrapper> & /*symbols*/){};
    virtual void subscriptionClosed(){};

    static std::unique_ptr<ObservableSubscriptionChangeListener> create();
};

struct DXFCPP_EXPORT ObservableSubscriptionChangeListenerImpl : ObservableSubscriptionChangeListener {

};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()