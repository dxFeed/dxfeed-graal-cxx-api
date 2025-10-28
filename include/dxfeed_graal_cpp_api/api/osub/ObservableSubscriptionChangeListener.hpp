// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../entity/SharedEntity.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

/**
 * The listener interface for receiving notifications on the changes of observed subscription.
 * All methods on this interface are invoked while holding a lock on the ObservableSubscription instance,
 * thus all changes for a given subscription are synchronized with respect to each other.
 */
struct DXFCPP_EXPORT ObservableSubscriptionChangeListener : RequireMakeShared<ObservableSubscriptionChangeListener> {
    class Key {
        Key() = default;
        friend class DXFeedSubscription;
        friend struct DXPublisherObservableSubscription;
    };

    explicit ObservableSubscriptionChangeListener(LockExternalConstructionTag);
    ~ObservableSubscriptionChangeListener() noexcept override;

    /**
     * Creates a listener that will notify the callback about added symbols.
     *
     * @param onSymbolsAdded A callback that will handle the event when symbols are added.
     * @return The new listener.
     */
    static std::shared_ptr<ObservableSubscriptionChangeListener>
    create(std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded);

    /**
     * Creates a listener that will notify callbacks about events of adding and deleting symbols, as well as about the
     * event of closing a subscription.
     * @param onSymbolsAdded A callback that will handle the symbol addition event.
     * @param onSymbolsRemoved A callback that will handle the symbol deletion event.
     * @param onSubscriptionClosed A callback that will handle the subscription close event.
     * @return The new listener.
     */
    static std::shared_ptr<ObservableSubscriptionChangeListener>
    create(std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded,
           std::function<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved,
           std::function<void()> onSubscriptionClosed);

    const JavaObjectHandle<ObservableSubscriptionChangeListener> &getHandle(Key) const;

    private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<ObservableSubscriptionChangeListener> handle_{};
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded_{};
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved_{};
    SimpleHandler<void()> onSubscriptionClosed_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()