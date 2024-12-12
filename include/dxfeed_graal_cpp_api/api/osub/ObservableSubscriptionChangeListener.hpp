// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../symbols/SymbolWrapper.hpp"
#include "../../entity/SharedEntity.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT ObservableSubscriptionChangeListener : RequireMakeShared<ObservableSubscriptionChangeListener> {
    explicit ObservableSubscriptionChangeListener(LockExternalConstructionTag);
  ~ObservableSubscriptionChangeListener() noexcept override;

    static std::shared_ptr<ObservableSubscriptionChangeListener>
    create(std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded);

    static std::shared_ptr<ObservableSubscriptionChangeListener>
    create(std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded,
           std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved,
           std::function<void()> onSubscriptionClosed);

    const JavaObjectHandle<ObservableSubscriptionChangeListener> &getHandle() const;

  private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<ObservableSubscriptionChangeListener> handle_;
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded_{};
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved_{};
    SimpleHandler<void()> onSubscriptionClosed_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()