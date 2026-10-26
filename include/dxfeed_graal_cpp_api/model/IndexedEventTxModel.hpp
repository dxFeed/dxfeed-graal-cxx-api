// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../api/SubscriptionController.hpp"
#include "../api/osub/IndexedEventSubscriptionSymbol.hpp"
#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/EventType.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../event/IndexedEvent.hpp"
#include "../event/IndexedEventSource.hpp"
#include "../internal/CallbackBarrier.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../internal/utils/StringUtils.hpp"
#include "../util/TimePeriod.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * \addtogroup dxfcpp_model
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;

struct DXFCPP_EXPORT IndexedEventTxModelTag {};
struct DXFCPP_EXPORT IndexedEventTxModelBuilderTag {};
struct DXFCPP_EXPORT IndexedEventTxModelListenerTag {};

/**
 * The part of IndexedEventTxModel that does not depend on the type of events.
 */
struct DXFCPP_EXPORT IndexedEventTxModelImpl {
    /// The function that receives the events of the model.
    using EventsReceived = std::function<void(const std::vector<std::shared_ptr<EventType>> &, bool)>;

    /// The listener of a model: the native listener and the function it calls (defined in the library).
    struct Listener;

    static JavaObjectHandle<IndexedEventTxModelBuilderTag> newBuilderImpl(const EventTypeEnum &eventType);

    static void withFeedImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                             const std::shared_ptr<DXFeed> &feed);

    static void withSymbolImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                               const IndexedEventSubscriptionSymbol &symbol);

    static void withSymbolImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                               const StringLike &symbol);

    static void withSourceImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                               const EventSourceWrapper &source);

    static std::shared_ptr<Listener> withListenerImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                                      EventsReceived eventsReceived);

    static void withEventsBatchLimitImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                         std::int32_t eventsBatchLimit);

    static void withAggregationPeriodImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder,
                                          std::int64_t aggregationPeriod);

    static void withExecutorImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder, void *executor);

    static JavaObjectHandle<IndexedEventTxModelTag>
    buildImpl(const JavaObjectHandle<IndexedEventTxModelBuilderTag> &builder);

    static IndexedEventSubscriptionSymbol
    getIndexedEventSubscriptionSymbolImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);

    static std::string getSymbolImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);

    static EventSourceWrapper getSourceImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);

    static std::shared_ptr<SubscriptionController>
    getSubscriptionControllerImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);

    static bool isClosedImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);

    static void closeImpl(const JavaObjectHandle<IndexedEventTxModelTag> &model);
};

/**
 * An incremental model of the indexed events of one symbol and one source (the `IndexedEventTxModel` of dxFeed
 * Java API, an experimental API there). The model delivers the events of complete snapshots and transactions only:
 * it delays the events of an incomplete snapshot or of an ongoing transaction until the snapshot is complete or the
 * transaction has ended.
 *
 * <h3>IndexedEventTxModel or IndexedTxModel</h3>
 *
 * - IndexedEventTxModel is the model of dxFeed Java API: one source, the events batch limit, the aggregation period
 *   and the executor of the underlying subscription, which can be changed later through the
 *   @ref IndexedEventTxModel::getSubscriptionController() "subscription controller" (it also attaches the model to
 *   and detaches it from a feed).
 * - IndexedTxModel is the model of the Graal SDK: several sources, which can be changed later, batch and snapshot
 *   processing flags, attach and detach on the model itself.
 *
 * <h3>Configuration</h3>
 *
 * The model is configured with the @ref IndexedEventTxModel::Builder "builder": the symbol, the source and the
 * listener (required), and the feed to attach to. The symbol and the source cannot be changed after the model is built.
 *
 * <h3>Listener and resource management</h3>
 *
 * The listener receives the events of one or more complete transactions and whether they form a snapshot. It is never
 * called concurrently with itself. By default, it is called by the executor of the endpoint of the feed.
 *
 * The model is closed by @ref IndexedEventTxModel::close() "close" or by the destructor: a closed model is
 * permanently detached from all feeds. The listener is released with the last of its owners: the models built with it
 * and the builder that holds it. Releasing it waits for its calls in progress on other threads, so do not destroy the
 * last owner while holding a lock that the listener takes. Until then, a call that was already scheduled may still
 * run after the model is closed. A listener must not own its model (for example, capture its `std::shared_ptr`): such
 * a model is never destroyed.
 *
 * ```cpp
 * auto model = IndexedEventTxModel<Order>::newBuilder()
 *                  ->withFeed(DXFeed::getInstance())
 *                  ->withSymbol("AAPL")
 *                  ->withSource(OrderSource::NTV)
 *                  ->withListener([](const auto &orders, bool isSnapshot) {
 *                      std::cout << (isSnapshot ? "Snapshot: " : "Update: ") << orders.size() << std::endl;
 *                  })
 *                  ->build();
 * ```
 *
 * @tparam E The type of events (derived from IndexedEvent).
 */
template <Derived<IndexedEvent> E>
struct /* DXFCPP_EXPORT */ IndexedEventTxModel final : IndexedEventTxModelImpl,
                                                       RequireMakeShared<IndexedEventTxModel<E>> {
    /// The function that receives the events of one or more complete transactions and whether they form a snapshot.
    using EventsListener =
        std::function<void(const std::vector<std::shared_ptr<E>> & /* events */, bool /* isSnapshot */)>;

    /**
     * The builder of IndexedEventTxModel. Each `with*` method changes this builder and returns it.
     */
    struct /* DXFCPP_EXPORT */ Builder final : RequireMakeShared<Builder> {
        private:
        JavaObjectHandle<IndexedEventTxModelBuilderTag> handle_;
        std::shared_ptr<Listener> listener_;

        public:
        Builder(typename Builder::LockExternalConstructionTag, JavaObjectHandle<IndexedEventTxModelBuilderTag> &&handle)
            : handle_(std::move(handle)) {
        }

        /**
         * Sets the feed to attach the model to. By default, the model is not attached to any feed.
         *
         * @param feed The feed.
         * @return this builder.
         */
        std::shared_ptr<Builder> withFeed(const std::shared_ptr<DXFeed> &feed) {
            withFeedImpl(handle_, feed);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the symbol and the source of the model.
         *
         * @param symbol The symbol with the source.
         * @return this builder.
         */
        std::shared_ptr<Builder> withSymbol(const IndexedEventSubscriptionSymbol &symbol) {
            withSymbolImpl(handle_, symbol);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the symbol of the model.
         *
         * @param symbol The symbol.
         * @return this builder.
         */
        std::shared_ptr<Builder> withSymbol(const StringLike &symbol) {
            withSymbolImpl(handle_, symbol);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the source of the model (required, unless it is set with the symbol). Use IndexedEventSource::DEFAULT
         * for the events that have no sources (for example, Series) and the source of the type of events for the
         * others (for example, OrderSource::NTV or OrderSource::DEFAULT for Order).
         *
         * @param source The source.
         * @return this builder.
         */
        std::shared_ptr<Builder> withSource(const EventSourceWrapper &source) {
            withSourceImpl(handle_, source);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the listener of the model (required). The listener cannot be changed after the model is built.
         *
         * @param listener The function that receives the events of one or more complete transactions and whether
         * they form a snapshot.
         * @return this builder.
         */
        std::shared_ptr<Builder> withListener(EventsListener listener) {
            if (!listener) {
                // An empty listener is rejected by withListenerImpl.
                listener_ = withListenerImpl(handle_, {});

                return this->template sharedAs<Builder>();
            }

            listener_ = withListenerImpl(
                handle_,
                [l = std::move(listener)](const std::vector<std::shared_ptr<EventType>> &events, bool isSnapshot) {
                    std::vector<std::shared_ptr<E>> filteredEvents{};

                    filteredEvents.reserve(events.size());

                    for (const auto &e : events) {
                        if (auto expected = e->template sharedAs<E>(); expected) {
                            filteredEvents.emplace_back(std::move(expected));
                        }
                    }

                    l(filteredEvents, isSnapshot);
                });

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the maximum number of events in a single notification of the listener
         * (DXFeedSubscription::OPTIMAL_BATCH_LIMIT by default).
         *
         * @param eventsBatchLimit The events batch limit.
         * @return this builder.
         */
        std::shared_ptr<Builder> withEventsBatchLimit(std::int32_t eventsBatchLimit) {
            withEventsBatchLimitImpl(handle_, eventsBatchLimit);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the aggregation period of the events (no aggregation by default).
         *
         * @param aggregationPeriod The aggregation period.
         * @return this builder.
         * @see SubscriptionController::getAggregationPeriod()
         */
        std::shared_ptr<Builder> withAggregationPeriod(const TimePeriod &aggregationPeriod) {
            return withAggregationPeriod(aggregationPeriod.getTime());
        }

        /**
         * Sets the aggregation period of the events (no aggregation by default).
         *
         * @param aggregationPeriod The aggregation period.
         * @return this builder.
         * @see SubscriptionController::getAggregationPeriod()
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::chrono::milliseconds aggregationPeriod) {
            return withAggregationPeriod(static_cast<std::int64_t>(aggregationPeriod.count()));
        }

        /**
         * Sets the aggregation period of the events (no aggregation by default).
         *
         * @param aggregationPeriod The aggregation period in milliseconds.
         * @return this builder.
         * @see SubscriptionController::getAggregationPeriod()
         */
        std::shared_ptr<Builder> withAggregationPeriod(std::int64_t aggregationPeriod) {
            withAggregationPeriodImpl(handle_, aggregationPeriod);

            return this->template sharedAs<Builder>();
        }

        /**
         * Sets the executor that notifies the listener (for example, InPlaceExecutor). By default, it is the
         * executor of the endpoint of the feed.
         *
         * @tparam Executor The type of the executor.
         * @param executor The executor.
         * @return this builder.
         */
        template <typename Executor> std::shared_ptr<Builder> withExecutor(const std::shared_ptr<Executor> &executor) {
            withExecutorImpl(handle_, executor ? executor->getHandle().get() : nullptr);

            return this->template sharedAs<Builder>();
        }

        /**
         * Builds the model and attaches it to the feed, if it is set. The models built by one builder share its
         * listener.
         *
         * @return a new model.
         */
        std::shared_ptr<IndexedEventTxModel> build() const {
            return IndexedEventTxModel::createShared(buildImpl(handle_), listener_);
        }
    };

    private:
    JavaObjectHandle<IndexedEventTxModelTag> handle_;
    // Destroyed before the model handle: it deactivates the listener, so the listener is not called after that.
    std::shared_ptr<Listener> listener_;

    public:
    IndexedEventTxModel(typename IndexedEventTxModel::LockExternalConstructionTag,
                        JavaObjectHandle<IndexedEventTxModelTag> &&handle, std::shared_ptr<Listener> listener)
        : handle_(std::move(handle)), listener_(std::move(listener)) {
    }

    /// Closes the model and destructs it. If the model is the last owner of its listener, waits for the calls of the
    /// listener in progress on other threads.
    ~IndexedEventTxModel() noexcept override {
        runRelease("IndexedEventTxModel", [this] {
            close();
        });
    }

    /**
     * Creates a new builder of the model.
     *
     * @return a new builder.
     */
    static std::shared_ptr<Builder> newBuilder() {
        return Builder::createShared(newBuilderImpl(E::TYPE));
    }

    /**
     * Returns the symbol of the model with its source.
     *
     * @return the symbol with the source.
     */
    IndexedEventSubscriptionSymbol getIndexedEventSubscriptionSymbol() const {
        return getIndexedEventSubscriptionSymbolImpl(handle_);
    }

    /**
     * Returns the symbol of the model.
     *
     * @return the symbol.
     */
    std::string getSymbol() const {
        return getSymbolImpl(handle_);
    }

    /**
     * Returns the source of the model.
     *
     * @return the source.
     */
    EventSourceWrapper getSource() const {
        return getSourceImpl(handle_);
    }

    /**
     * Returns the controller of the underlying subscription of the model: it attaches the model to and detaches it
     * from a feed and changes the events batch limit, the aggregation period and the executor.
     *
     * @return the subscription controller.
     */
    std::shared_ptr<SubscriptionController> getSubscriptionController() const {
        return getSubscriptionControllerImpl(handle_);
    }

    /**
     * Returns whether the model is closed.
     *
     * @return `true` if the model is closed.
     */
    bool isClosed() const {
        return isClosedImpl(handle_);
    }

    /**
     * Closes the model: it is permanently detached from all feeds. The next calls of close() do nothing.
     */
    void close() const {
        closeImpl(handle_);
    }
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()
