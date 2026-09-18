// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../entity/SharedEntity.hpp"
#include "../../internal/Id.hpp"
#include "../../symbols/SymbolWrapper.hpp"

#include <functional>
#include <memory>
#include <unordered_set>

/**
 * \addtogroup dxfcpp_osub
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The listener interface for receiving notifications on the changes of observed subscription.
 * All methods on this interface are invoked while holding a lock on the ObservableSubscription instance,
 * thus all changes for a given subscription are synchronized with respect to each other.
 */
struct DXFCPP_EXPORT ObservableSubscriptionChangeListener : RequireMakeShared<ObservableSubscriptionChangeListener> {
    class Key {
        Key() {};
        friend class DXFeedSubscription;
        friend struct DXPublisherObservableSubscription;
    };

    explicit ObservableSubscriptionChangeListener(LockExternalConstructionTag);

    /**
     * Destroys this listener and invalidates its native callback registration context.
     *
     * Subscriptions that still have the listener installed keep it alive, so destruction cannot race with a callback
     * that has successfully acquired the listener.
     */
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

    /**
     * Returns the native listener handle to an owning subscription implementation.
     *
     * @return The native listener handle.
     */
    const JavaObjectHandle<ObservableSubscriptionChangeListener> &getHandle(Key) const;

    /**
     * Delivers an added-symbol notification from an owning subscription implementation.
     *
     * @param symbols The symbols added to the subscription.
     */
    void notifySymbolsAdded(Key, const std::unordered_set<SymbolWrapper> &symbols);

    /**
     * Delivers a removed-symbol notification from an owning subscription implementation.
     *
     * @param symbols The symbols removed from the subscription.
     */
    void notifySymbolsRemoved(Key, const std::unordered_set<SymbolWrapper> &symbols);

    /** Delivers a subscription-closed notification from an owning subscription implementation. */
    void notifySubscriptionClosed(Key);

    private:
    mutable std::recursive_mutex mutex_{};
    Id<ObservableSubscriptionChangeListener> id_{Id<ObservableSubscriptionChangeListener>::UNKNOWN};
    JavaObjectHandle<ObservableSubscriptionChangeListener> handle_{};
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsAdded_{};
    SimpleHandler<void(const std::unordered_set<SymbolWrapper> &symbols)> onSymbolsRemoved_{};
    SimpleHandler<void()> onSubscriptionClosed_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
