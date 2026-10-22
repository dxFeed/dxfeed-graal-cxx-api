// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../util/TimePeriod.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>

/**
 * \addtogroup dxfcpp_api
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;

/**
 * Controls the subscription of an object that owns an underlying DXFeedSubscription (for example,
 * IndexedEventTxModel): attaches it to and detaches it from a feed and changes the settings of the event delivery.
 *
 * ```cpp
 * auto controller = model->getSubscriptionController();
 *
 * controller->setAggregationPeriod(std::chrono::milliseconds(100));
 * controller->detach(feed);
 * ```
 */
struct DXFCPP_EXPORT SubscriptionController final : RequireMakeShared<SubscriptionController> {
    /// The alias to a type of shared pointer to the SubscriptionController object
    using Ptr = std::shared_ptr<SubscriptionController>;

    private:
    JavaObjectHandle<SubscriptionController> handle_;

    void setExecutorImpl(void *executor) const;

    public:
    explicit SubscriptionController(RequireMakeShared<SubscriptionController>::LockExternalConstructionTag,
                                    JavaObjectHandle<SubscriptionController> &&handle);

    ~SubscriptionController() noexcept override;

    /**
     * Attaches the underlying subscription to the given feed.
     *
     * @param feed The feed to attach to.
     */
    void attach(const std::shared_ptr<DXFeed> &feed) const;

    /**
     * Detaches the underlying subscription from the given feed.
     *
     * @param feed The feed to detach from.
     */
    void detach(const std::shared_ptr<DXFeed> &feed) const;

    /**
     * Returns the maximum number of events in a single notification of the listener.
     *
     * @return the events batch limit (DXFeedSubscription::OPTIMAL_BATCH_LIMIT is the default one).
     */
    std::int32_t getEventsBatchLimit() const;

    /**
     * Sets the maximum number of events in a single notification of the listener.
     *
     * @param eventsBatchLimit The events batch limit: DXFeedSubscription::OPTIMAL_BATCH_LIMIT,
     * DXFeedSubscription::MAX_BATCH_LIMIT or a positive number.
     */
    void setEventsBatchLimit(std::int32_t eventsBatchLimit) const;

    /**
     * Returns the aggregation period of the events: the listener is notified at most once per this period, and
     * the events of the same symbol within the period may be conflated.
     *
     * @return the aggregation period; TimePeriod::ZERO when it is not set (then only the aggregation period of the
     * feed applies).
     */
    TimePeriod getAggregationPeriod() const;

    /**
     * Sets the aggregation period of the events.
     *
     * @param aggregationPeriod The aggregation period.
     * @see getAggregationPeriod()
     */
    void setAggregationPeriod(const TimePeriod &aggregationPeriod) const;

    /**
     * Sets the aggregation period of the events.
     *
     * @param aggregationPeriod The aggregation period.
     * @see getAggregationPeriod()
     */
    void setAggregationPeriod(std::chrono::milliseconds aggregationPeriod) const;

    /**
     * Sets the aggregation period of the events.
     *
     * @param aggregationPeriod The aggregation period in milliseconds.
     * @see getAggregationPeriod()
     */
    void setAggregationPeriod(std::int64_t aggregationPeriod) const;

    /**
     * Sets the executor that notifies the listener (for example, InPlaceExecutor).
     *
     * @tparam Executor The type of the executor.
     * @param executor The executor, or `nullptr` to use the executor of the endpoint of the feed.
     */
    template <typename Executor> void setExecutor(const std::shared_ptr<Executor> &executor) const {
        setExecutorImpl(executor ? executor->getHandle().get() : nullptr);
    }

    /**
     * Makes the executor of the endpoint of the feed notify the listener.
     */
    void setExecutor(std::nullptr_t) const;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
