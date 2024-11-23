// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/IndexedEvent.hpp"
#include "../event/market/OrderSource.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "TxModelListener.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct SymbolWrapper;
struct DXFeed;

/**
 * An incremental model for indexed events.
 * This model manages all snapshot and transaction logic, subscription handling, and listener notifications.
 *
 * <p>This model is designed to handle incremental transactions. Users of this model only see the list
 * of events in a consistent state. This model delays incoming events that are part of an incomplete snapshot
 * or ongoing transaction until the snapshot is complete or the transaction has ended. This model notifies
 * the user of received transactions through an installed @ref TxModelListener "listener".
 *
 * <h3>Configuration</h3>
 *
 * <p>This model must be configured using the @ref IndexedTxModel::Builder "builder", as most configuration
 * settings cannot be changed once the model is built. This model requires configuration with
 * a @ref IndexedTxModel::Builder::Builder#withSymbol() "symbol" and a @ref IndexedTxModel::Builder::withSources()
 * sources for subscription, and it must be @ref IndexedTxModel::Builder::withFeed() "attached" to a DXFeed instance to
 * begin operation. For ease of use, some of these configurations can be changed after the model is built, see
 * @ref IndexedTxModel::setSources() "setSources" and @ref IndexedTxModel::attach() "attach" and their overloads.
 *
 * <p>This model only supports single symbol subscriptions; multiple symbols cannot be configured.
 *
 * <h3>Resource management and closed models</h3>
 *
 * <p>Attached model is a potential memory leak. If the pointer to attached model is lost, then there is no way
 * to detach this model from the feed and the model will not be reclaimed by the garbage collector as long as the
 * corresponding feed is still used. Detached model can be reclaimed by the garbage collector, but detaching model
 * requires knowing the pointer to the feed at the place of the call, which is not always convenient.
 *
 * <p>The convenient way to detach model from the feed is to call its @ref IndexedTxModel::close "close" method
 * (it is automatically called when the destructor is called, i.e. RAII). Closed model
 * becomes permanently detached from all feeds, removes all its listeners and is guaranteed to be reclaimable by
 * the garbage collector as soon as all external references to it are cleared.
 *
 * <h3>Threads and locks</h3>
 *
 * <p>This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 *
 * Sample:
 *
 * ```cpp
 * auto feed = DXEndpoint::getInstance(DXEndpoint::Role::FEED)->connect("demo.dxfeed.com:7300")->getFeed();
 * auto listener = TxModelListener<Order>::create([](const auto &, const auto &events, bool isSnapshot) {
 *     if (isSnapshot) {
 *         std::cout << "Snapshot:" << std::endl;
 *     } else {
 *         std::cout << "Update:" << std::endl;
 *     }
 *
 *     for (const auto &e : events) {
 *         std::cout << "[" << e->getEventFlagsMask().toString() << "]:" << e << std::endl;
 *     }
 *
 *     std::cout << std::endl;
 * });
 * auto model = IndexedTxModel::newBuilder(Order::TYPE)
 *                  ->withFeed(feed)
 *                  ->withBatchProcessing(true)
 *                  ->withSnapshotProcessing(true)
 *                  ->withSources({OrderSource::NTV, OrderSource::DEX})
 *                  ->withListener(listener)
 *                  ->withSymbol("IBM")
 *                  ->build();
 *
 * std::this_thread::sleep_for(10s);
 * ```
 */
struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    /**
     * A builder class for creating an instance of IndexedTxModel.
     */
    struct DXFCPP_EXPORT Builder : RequireMakeShared<Builder> {
      private:
        JavaObjectHandle<Builder> handle_;
        std::shared_ptr<TxModelListenerCommon> listener_;

        JavaObjectHandle<Builder> withSourcesImpl(void *graalEventSourceList) const;
        JavaObjectHandle<Builder> withListenerImpl(const JavaObjectHandle<TxModelListenerTag> &listener) const;

      public:
        Builder(LockExternalConstructionTag tag, JavaObjectHandle<Builder> &&handle);

        Builder(LockExternalConstructionTag tag, JavaObjectHandle<Builder> &&handle,
                std::shared_ptr<TxModelListenerCommon> listener);

        /**
         * Enables or disables batch processing.
         * <b>This is enabled by default</b>.
         *
         * <p>If batch processing is disabled, the model will notify listener
         * <b>separately for each transaction</b> (even if it is represented by a single event);
         * otherwise, transactions can be combined in a single listener call.
         *
         * <p>A transaction may represent either a snapshot or update events that are received after a snapshot.
         * Whether this flag is set or not, the model will always notify listeners that a snapshot has been received
         * and will not combine multiple snapshots or a snapshot with another transaction
         * into a single listener notification.
         *
         * @param isBatchProcessing `true` to enable batch processing; `false` otherwise.
         * @return `this` builder.
         */
        std::shared_ptr<Builder> withBatchProcessing(bool isBatchProcessing) const;

        /**
         * Enables or disables snapshot processing.
         * <b>This is disabled by default</b>.
         *
         * <p>If snapshot processing is enabled, transactions representing a snapshot will be processed as follows:
         * events that are marked for removal will be removed, repeated indexes will be merged, and
         * @ref IndexedEvent::getEventFlags() "eventFlags" of events are set to zero;
         * otherwise, the user will see the snapshot in raw form, with possible repeated indexes,
         * events marked for removal, and @ref IndexedEvent::getEventFlags() "eventFlags" unchanged.
         *
         * <p>Whether this flag is set or not, in transactions that are not a snapshot, events that are marked
         * for removal will not be removed, repeated indexes will not be merged, and
         * @ref IndexedEvent::getEventFlags() "eventFlags" of events will not be changed.
         * This flag only affects the processing of transactions that are a snapshot.
         *
         * @param isSnapshotProcessing `true` to enable snapshot processing; `false` otherwise.
         * @return `this` builder.
         */
        std::shared_ptr<Builder> withSnapshotProcessing(bool isSnapshotProcessing) const;

        /**
         * Sets the @ref DXFeed "feed" for the model being created.
         * The @ref DXFeed "feed" can also be attached later, after the model has been created,
         * by calling @ref IndexedTxModel::attach() "attach".
         *
         * @param feed The @ref DXFeed "feed".
         * @return `this` builder.
         */
        std::shared_ptr<Builder> withFeed(std::shared_ptr<DXFeed> feed) const;

        /**
         * Sets the subscription symbol for the model being created.
         * The symbol cannot be added or changed after the model has been built.
         *
         * @param symbol The subscription symbol.
         * @return `this` builder.
         */
        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol) const;

        /**
         * Sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * ```cpp
         * auto listener = TxModelListener<Order>::create([](const auto &, const auto &events, bool isSnapshot) {
         *     if (isSnapshot) {
         *         std::cout << "Snapshot:" << std::endl;
         *     } else {
         *         std::cout << "Update:" << std::endl;
         *     }
         *
         *     for (const auto &e : events) {
         *         std::cout << "[" << e->getEventFlagsMask().toString() << "]:" << e << std::endl;
         *     }
         *
         *     std::cout << std::endl;
         * });
         *
         * builder->withListener(listener);
         * ```
         *
         * @param listener The transaction listener.
         * @return `this` builder.
         */
        template <Derived<IndexedEvent> E>
        std::shared_ptr<Builder> withListener(std::shared_ptr<TxModelListener<E>> listener) const {
            return createShared(std::move(withListenerImpl(listener->getHandle())),
                                listener->template sharedAs<TxModelListenerCommon>());
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * <p>The default value for this source is **`empty`**,
         * which means that this model subscribes to all available sources.
         * These sources can be changed later, after the model has been created,
         * by calling @ref IndexedTxModel::setSources() "setSources".
         *
         * ```cpp
         * std::vector<EventSourceWrapper> sources{OrderSource::NTV, OrderSource::DEX};
         *
         * builder->withSources(sources.begin(), sources.end());
         * ```
         *
         * @tparam EventSourceIt The source collection iterator type.
         * @param begin The beginning of the collection of sources.
         * @param end The end of the collection of sources.
         * @return `this` builder.
         */
        template <typename EventSourceIt>
        std::shared_ptr<Builder> withSources(EventSourceIt begin, EventSourceIt end) const {
            auto list = EventSourceWrapper::ListUtils::toGraalListUnique(begin, end);

            return createShared(std::move(withSourcesImpl(list.get())));
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * <p>The default value for this source is **`empty`**,
         * which means that this model subscribes to all available sources.
         * These sources can be changed later, after the model has been created,
         * by calling @ref IndexedTxModel::setSources() "setSources".
         *
         * ```cpp
         * builder->withSources(std::vector{OrderSource::NTV, OrderSource::DEX});
         * // or
         * builder->withSources(std::unordered_set{OrderSource::NTV, OrderSource::DEX});
         * ```
         *
         * @tparam EventSourceCollection A type of the collection of sources (std::vector<EventSourceWrapper>,
         * std::set<OrderSource>, etc.)
         * @param sources The specified sources.
         * @return `this` builder.
         */
        template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
        std::shared_ptr<Builder> withSources(EventSourceCollection &&sources) const {
            return withSources(std::begin(sources), std::end(sources));
        }

        /**
         * Sets the sources from which to subscribe for indexed events.
         * If no sources have been set, subscriptions will default to all possible sources.
         *
         * <p>The default value for this source is **`empty`**,
         * which means that this model subscribes to all available sources.
         * These sources can be changed later, after the model has been created,
         * by calling @ref IndexedTxModel::setSources() "setSources".
         *
         * ```cpp
         * builder->withSources({OrderSource::NTV, OrderSource::DEX});
         * builder->withSources({OrderSource::NTV});
         * ```
         *
         * @param sources The specified sources.
         * @return `this` builder.
         */
        std::shared_ptr<Builder> withSources(std::initializer_list<EventSourceWrapper> sources) const;

        /**
         * Builds an instance of IndexedTxModel based on the provided parameters.
         *
         * ```cpp
         * auto model = builder->build();
         * ```
         *
         * @return The created IndexedTxModel.
         */
        std::shared_ptr<IndexedTxModel> build() const;
    };

  private:
    JavaObjectHandle<IndexedTxModel> handle_;
    std::shared_ptr<TxModelListenerCommon> listener_;

    void setSourcesImpl(void *graalEventSourceList) const;

  public:
    IndexedTxModel(LockExternalConstructionTag tag, JavaObjectHandle<IndexedTxModel> &&handle);

    IndexedTxModel(LockExternalConstructionTag tag, JavaObjectHandle<IndexedTxModel> &&handle,
                   std::shared_ptr<TxModelListenerCommon> listener);

    /// Calls @ref IndexedTxModel::close "close" method and destructs this model.
    ~IndexedTxModel() noexcept override;

    /**
     * Factory method to create a new builder for this model.
     *
     * ```cpp
     * auto builder = model->newBuilder(Order::TYPE);
     * ```
     *
     * @param eventType The class type of time series event.
     * @return A new @ref IndexedTxModel::Builder "builder instance.
     */
    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum &eventType);

    /**
     * Returns whether batch processing is enabled.
     * See @ref IndexedTxModel::Builder::withBatchProcessing() "withBatchProcessing".
     *
     * @return `true` if batch processing is enabled; `false` otherwise.
     */
    bool isBatchProcessing() const;

    /**
     * Returns whether snapshot processing is enabled.
     * See @ref IndexedTxModel::Builder::withSnapshotProcessing() "withSnapshotProcessing".
     *
     * @return `true` if snapshot processing is enabled; `false` otherwise.
     */
    bool isSnapshotProcessing() const;

    /**
     * Attaches this model to the specified feed.
     * Technically, this model can be attached to multiple feeds at once,
     * but this is rarely needed and not recommended.
     *
     * @param feed The feed to attach to.
     */
    void attach(std::shared_ptr<DXFeed> feed) const;

    /**
     * Detaches this model from the specified feed.
     *
     * @param feed The feed to detach from.
     */
    void detach(std::shared_ptr<DXFeed> feed) const;

    /**
     * Closes this model and makes it <i>permanently detached</i>.
     *
     * <p>This method clears installed listener and ensures that the model
     * can be safely garbage-collected when all outside references to it are lost.
     */
    void close() const;

    /**
     * Returns the current set of sources.
     * If no sources have been set, an empty set is returned, indicating that all possible sources have been subscribed
     * to.
     *
     * @return The set of current sources.
     */
    std::unordered_set<EventSourceWrapper> getSources() const;

    /**
     * Sets the sources from which to subscribe for indexed events.
     * If an empty list is provided, subscriptions will default to all available sources.
     * If these sources have already been set, nothing happens.
     *
     * ```cpp
     * std::vector<EventSourceWrapper> sources{OrderSource::NTV, OrderSource::DEX};
     *
     * model->setSources(sources.begin(), sources.end());
     * ```
     *
     * @tparam EventSourceIt The source collection iterator type.
     * @param begin The beginning of the collection of sources.
     * @param end The end of the collection of sources.
     */
    template <typename EventSourceIt> void setSources(EventSourceIt begin, EventSourceIt end) const {
        auto list = EventSourceWrapper::ListUtils::toGraalListUnique(begin, end);

        setSourcesImpl(list.get());
    }

    /**
     * Sets the sources from which to subscribe for indexed events.
     * If an empty list is provided, subscriptions will default to all available sources.
     * If these sources have already been set, nothing happens.
     *
     * ```cpp
     * model->setSources(std::vector{OrderSource::NTV, OrderSource::DEX});
     * // or
     * model->setSources(std::unordered_set{OrderSource::NTV, OrderSource::DEX});
     * ```
     *
     * @tparam EventSourceCollection A type of the collection of sources (std::vector<EventSourceWrapper>,
     * std::set<OrderSource>, etc.)
     * @param sources The specified sources.
     */
    template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
    void setSources(EventSourceCollection &&sources) const {
        setSources(std::begin(sources), std::end(sources));
    }

    /**
     * Sets the sources from which to subscribe for indexed events.
     * If an empty list is provided, subscriptions will default to all available sources.
     * If these sources have already been set, nothing happens.
     *
     * ```cpp
     * model->setSources({OrderSource::NTV, OrderSource::DEX});
     * model->setSources({OrderSource::NTV});
     * ```
     *
     * @param sources The specified sources.
     */
    void setSources(std::initializer_list<EventSourceWrapper> sources) const;
};

DXFCPP_END_NAMESPACE