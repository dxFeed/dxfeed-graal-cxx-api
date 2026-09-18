// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/EntityModule.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "./osub/ObservableSubscription.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

/**
 * \addtogroup dxfcpp_api
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct ObservableSubscriptionChangeListener;

struct DXFCPP_EXPORT DXPublisherObservableSubscription : RequireMakeShared<DXPublisherObservableSubscription>,
                                                         ObservableSubscription {
    static constexpr std::size_t FAKE_LISTENER_ID{static_cast<std::size_t>(-1)};

    private:
    inline static std::atomic<std::size_t> lastListenerId_{};

    /** Owns the public listener and the per-subscription native callback bridge installed for it. */
    struct ListenerRegistration {
        std::shared_ptr<ObservableSubscriptionChangeListener> listener;
        std::shared_ptr<ObservableSubscriptionChangeListener> bridge;
    };

    JavaObjectHandle<DXPublisherObservableSubscription> handle_;
    std::unordered_map<std::size_t, ListenerRegistration> listeners_;
    std::recursive_mutex listenersMutex_{};

    void releaseChangeListenerAfterClose(std::size_t listenerId);

    public:
    DXPublisherObservableSubscription(LockExternalConstructionTag,
                                      JavaObjectHandle<DXPublisherObservableSubscription> &&handle);

    /**
     * Removes native listener registrations and releases all listeners owned by this subscription.
     */
    ~DXPublisherObservableSubscription() noexcept override;

    static std::shared_ptr<DXPublisherObservableSubscription>
    create(JavaObjectHandle<DXPublisherObservableSubscription> &&handle);

    bool isClosed() override;
    std::unordered_set<EventTypeEnum> getEventTypes() override;
    bool containsEventType(const EventTypeEnum &eventType) override;
    std::size_t addChangeListener(std::shared_ptr<ObservableSubscriptionChangeListener> listener) override;
    void removeChangeListener(std::size_t changeListenerId) override;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
