// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventTypeEnum.hpp"
#include "../event/TimeSeriesEvent.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "../symbols/SymbolWrapper.hpp"
#include "TxModelListener.hpp"

#include <chrono>
#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;

struct DXFCPP_EXPORT TimeSeriesTxModelTag{};
struct DXFCPP_EXPORT TimeSeriesTxModelBuilderTag{};

struct DXFCPP_EXPORT TimeSeriesTxModelImpl {
    struct DXFCPP_EXPORT Builder {
        protected:
        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withBatchProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle, bool isBatchProcessing);

        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withSnapshotProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                                   bool isSnapshotProcessing);

        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withFeedImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle, const std::shared_ptr<DXFeed> &feed);

        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withSymbolImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle, const SymbolWrapper &symbol);

        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withListenerImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle,
                         const JavaObjectHandle<TxModelListenerTag> &listener);

        static JavaObjectHandle<TimeSeriesTxModelBuilderTag>
        withFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle, std::int64_t fromTime);

        static JavaObjectHandle<TimeSeriesTxModelTag>
        buildImpl(const JavaObjectHandle<TimeSeriesTxModelBuilderTag> &handle);
    };

    protected:
    static JavaObjectHandle<TimeSeriesTxModelBuilderTag> newBuilderImpl(const EventTypeEnum &eventType);

    static bool isBatchProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle);

    static bool isSnapshotProcessingImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle);

    static void attachImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle, const std::shared_ptr<DXFeed> &feed);

    static void detachImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle, const std::shared_ptr<DXFeed> &feed);

    static void closeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle);

    static std::int64_t getFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle);

    static void setFromTimeImpl(const JavaObjectHandle<TimeSeriesTxModelTag> &handle, std::int64_t fromTime);
};

/**
 * An incremental model for time-series events.
 * This model manages all snapshot and transaction logic, subscription handling, and listener notifications.
 *
 * <p>This model is designed to handle incremental transactions. Users of this model only see the list
 * of events in a consistent state. This model delays incoming events that are part of an incomplete snapshot
 * or ongoing transaction until the snapshot is complete or the transaction has ended. This model notifies
 * the user of received transactions through an installed @ref TxModelListener "listener".
 *
 * <h3>Configuration</h3>
 *
 * <p>This model must be configured using the @ref TimeSeriesTxModel::Builder "builder", as most configuration
 * settings cannot be changed once the model is built. This model requires configuration with
 * a @ref TimeSeriesTxModel::Builder::withSymbol() "symbol" and a @ref TimeSeriesTxModel::Builder::withFromTime()
 * "fromTime" for subscription, and it must be @ref TimeSeriesTxModel::Builder::withFeed() "attached" to a DXFeed
 * instance to begin operation. For ease of use, some of these configurations can be changed after the model is built,
 * see
 * @ref TimeSeriesTxModel::setFromTime() "setFromTime", @ref TimeSeriesTxModel::attach() "attach".
 *
 * <p>This model only supports single symbol subscriptions; multiple symbols cannot be configured.
 *
 * <h3>Resource management and closed models</h3>
 *
 * <p>Attached model is a potential memory leak. If the pointer to attached model is lost, then there is no way
 * to detach this model from the feed.
 *
 * <p>The convenient way to detach model from the feed is to call its @ref TimeSeriesTxModel::close "close" method
 * (it is automatically called when the destructor is called, i.e. RAII). Closed model
 * becomes permanently detached from all feeds, removes all its listeners.
 *
 * <h3>Threads and locks</h3>
 *
 * <p>This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 *
 * Sample:
 *
 * ```cpp
 * auto feed = DXEndpoint::getInstance(DXEndpoint::Role::FEED)->connect("demo.dxfeed.com:7300")->getFeed();
 * auto listener = TimeSeriesTxModelListener<Candle>::create([](const auto &events, bool isSnapshot) {
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
 * auto model = TimeSeriesTxModel<Candle>::newBuilder()
 *                  ->withFeed(feed)
 *                  ->withBatchProcessing(true)
 *                  ->withSnapshotProcessing(true)
 *                  ->withFromTime(std::chrono::milliseconds(dxfcpp::now()) - std::chrono::days(3))
 *                  ->withListener(listener)
 *                  ->withSymbol(CandleSymbol::valueOf("AAPL&Q{=1m}"))
 *                  ->build();
 *
 * std::this_thread::sleep_for(10s);
 * ```
 * @tparam E The type of event (derived from TimeSeriesEvent)
 */
template <Derived<TimeSeriesEvent> E>
struct /* DXFCPP_EXPORT */ TimeSeriesTxModel final : TimeSeriesTxModelImpl, RequireMakeShared<TimeSeriesTxModel<E>> {

    /// A builder class for creating an instance of TimeSeriesTxModel.
    struct /* DXFCPP_EXPORT */ Builder final : TimeSeriesTxModelImpl::Builder, RequireMakeShared<Builder> {
        private:
        JavaObjectHandle<TimeSeriesTxModelBuilderTag> handle_;
        std::shared_ptr<TxModelListenerCommon> listener_;

        public:
        Builder(typename Builder::LockExternalConstructionTag, JavaObjectHandle<TimeSeriesTxModelBuilderTag> &&handle)
            : handle_(std::move(handle)) {
        }

        Builder(typename Builder::LockExternalConstructionTag, JavaObjectHandle<TimeSeriesTxModelBuilderTag> &&handle,
                std::shared_ptr<TxModelListenerCommon> listener)
            : handle_(std::move(handle)), listener_(std::move(listener)) {
        }

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
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withBatchProcessing(bool isBatchProcessing) const {
            return RequireMakeShared<Builder>::template createShared(
                std::move(withBatchProcessingImpl(handle_, isBatchProcessing)));
        }

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
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withSnapshotProcessing(bool isSnapshotProcessing) const {
            return RequireMakeShared<Builder>::template createShared(
                std::move(withSnapshotProcessingImpl(handle_, isSnapshotProcessing)));
        }

        /**
         * Sets the @ref DXFeed "feed" for the model being created.
         * The @ref DXFeed "feed" can also be attached later, after the model has been created,
         * by calling @ref TimeSeriesTxModel::attach() "attach".
         *
         * @param feed The @ref DXFeed "feed".
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFeed(const std::shared_ptr<DXFeed> &feed) const {
            return RequireMakeShared<Builder>::template createShared(std::move(withFeedImpl(handle_, feed)));
        }

        /**
         * Sets the subscription symbol for the model being created.
         * The symbol cannot be added or changed after the model has been built.
         *
         * @param symbol The subscription symbol.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol) const {
            return RequireMakeShared<Builder>::template createShared(std::move(withSymbolImpl(handle_, symbol)));
        }

        /**
         * Sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * ```cpp
         * auto listener = TimeSeriesTxModelListener<Candle>::create([](const auto &events, bool isSnapshot) {
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
         * @tparam E The type of event (derived from TimeSeriesEvent)
         * @param listener The transaction listener.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withListener(std::shared_ptr<TimeSeriesTxModelListener<E>> listener) const {
            return RequireMakeShared<Builder>::template createShared(
                std::move(withListenerImpl(handle_, listener->getHandle())),
                listener->template sharedAs<TxModelListenerCommon>());
        }

        /**
         * Sets the listener for transaction notifications.
         * The listener cannot be changed or added once the model has been built.
         *
         * ```cpp
         * auto builder = TimeSeriesTxModel<Candle>::newBuilder();
         *
         * builder = builder->withListener([](const auto &events, bool isSnapshot) {
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
         * ```
         *
         * @param onEventsReceived A functional object, lambda, or function to which time series event data will be
         * passed.
         * @return The builder instance.
         */
        std::shared_ptr<Builder>
        withListener(std::function<void(const std::vector<std::shared_ptr<E>> & /* events */, bool /* isSnapshot */)>
                         onEventsReceived) const {
            return withListener(TimeSeriesTxModelListener<E>::create(onEventsReceived));
        }

        /**
         * Sets the time from which to subscribe for time-series.
         *
         * <p>This time defaults to **`std::numeric_limits<std::int64_t>::max()`**, which means that this model is not
         * subscribed. This time can be changed later, after the model has been created, by calling @ref
         * TimeSeriesTxModel::setFromTime() "setFromTime".
         *
         * @param fromTime The time in milliseconds, since Unix epoch of January 1, 1970.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFromTime(std::int64_t fromTime) const {
            return RequireMakeShared<Builder>::template createShared(std::move(withFromTimeImpl(handle_, fromTime)));
        }

        /**
         * Sets the time from which to subscribe for time-series.
         *
         * <p>This time defaults to **`std::chrono::milliseconds(std::numeric_limits<std::int64_t>::max())`**, which
         * means that this model is not subscribed. This time can be changed later, after the model has been created, by
         * calling @ref TimeSeriesTxModel::setFromTime() "setFromTime".
         *
         * @param fromTime The duration in milliseconds, since Unix epoch of January 1, 1970.
         * @return The builder instance.
         */
        std::shared_ptr<Builder> withFromTime(std::chrono::milliseconds fromTime) const {
            return withFromTime(fromTime.count());
        }

        /**
         * Builds an instance of TimeSeriesTxModel based on the provided parameters.
         *
         * ```cpp
         * auto model = builder->build();
         * ```
         *
         * @return The created TimeSeriesTxModel.
         */
        std::shared_ptr<TimeSeriesTxModel> build() const {
            return RequireMakeShared<TimeSeriesTxModel>::template createShared(std::move(buildImpl(handle_)),
                                                                               listener_);
        }
    };

    private:
    JavaObjectHandle<TimeSeriesTxModelTag> handle_;
    std::shared_ptr<TxModelListenerCommon> listener_;

    public:
    TimeSeriesTxModel(typename TimeSeriesTxModel::LockExternalConstructionTag,
                      JavaObjectHandle<TimeSeriesTxModelTag> &&handle)
        : handle_(std::move(handle)) {
    }

    TimeSeriesTxModel(typename TimeSeriesTxModel::LockExternalConstructionTag,
                      JavaObjectHandle<TimeSeriesTxModelTag> &&handle, std::shared_ptr<TxModelListenerCommon> listener)
        : handle_(std::move(handle)), listener_(std::move(listener)) {
    }

    /// Calls @ref TimeSeriesTxModel::close "close" method and destructs this model.
    ~TimeSeriesTxModel() noexcept override {
        close();
    }

    /**
     * Factory method to create a new builder for this model.
     *
     * ```cpp
     * auto builder = model->newBuilder();
     * ```
     *
     * @return A new @ref TimeSeriesTxModel::Builder "builder instance.
     */
    static std::shared_ptr<Builder> newBuilder() {
        return RequireMakeShared<Builder>::template createShared(std::move(newBuilderImpl(E::TYPE)));
    }

    /**
     * Returns whether batch processing is enabled.
     * See @ref TimeSeriesTxModel::Builder::withBatchProcessing() "withBatchProcessing".
     *
     * @return `true` if batch processing is enabled; `false` otherwise.
     */
    bool isBatchProcessing() const {
        return isBatchProcessingImpl(handle_);
    }

    /**
     * Returns whether snapshot processing is enabled.
     * See @ref TimeSeriesTxModel::Builder::withSnapshotProcessing() "withSnapshotProcessing".
     *
     * @return `true` if snapshot processing is enabled; `false` otherwise.
     */
    bool isSnapshotProcessing() const {
        return isSnapshotProcessingImpl(handle_);
    }

    /**
     * Attaches this model to the specified feed.
     * Technically, this model can be attached to multiple feeds at once,
     * but this is rarely needed and not recommended.
     *
     * @param feed The feed to attach to.
     */
    void attach(const std::shared_ptr<DXFeed> &feed) const {
        attachImpl(handle_, feed);
    }

    /**
     * Detaches this model from the specified feed.
     *
     * @param feed The feed to detach from.
     */
    void detach(const std::shared_ptr<DXFeed> &feed) const {
        detachImpl(handle_, feed);
    }

    /**
     * Closes this model and makes it <i>permanently detached</i>.
     *
     * <p>This method clears installed listener.
     */
    void close() const {
        closeImpl(handle_);
    }

    /**
     * Returns the time from which to subscribe for time-series,
     * or **`std::numeric_limits<std::int64_t>::max()`** if this model is not subscribed (this is a default value).
     *
     * @return The time in milliseconds, since Unix epoch of January 1, 1970.
     */
    std::int64_t getFromTime() const {
        return getFromTimeImpl(handle_);
    }

    /**
     * Sets the time from which to subscribe for time-series.
     * If this time has already been set, nothing happens.
     *
     * @param fromTime The time in milliseconds, since Unix epoch of January 1, 1970.
     */
    void setFromTime(std::int64_t fromTime) const {
        setFromTimeImpl(handle_, fromTime);
    }

    /**
     * Sets the time from which to subscribe for time-series.
     * If this time has already been set, nothing happens.
     *
     * @param fromTime The duration in milliseconds, since Unix epoch of January 1, 1970.
     */
    void setFromTime(std::chrono::milliseconds fromTime) const {
        setFromTime(fromTime.count());
    }

    std::string toString() const override {
        return JavaObject::toString(handle_.get());
    }

    friend std::ostream &operator<<(std::ostream &os, const TimeSeriesTxModel &m) {
        return os << m.toString();
    }

    std::size_t hashCode() const noexcept {
        return JavaObject::hashCode(handle_.get());
    }

    bool operator==(const TimeSeriesTxModel &other) const {
        return JavaObject::equals(handle_.get(), other.handle_.get());
    }
};

DXFCPP_END_NAMESPACE

template <dxfcpp::Derived<dxfcpp::TimeSeriesEvent> E> struct std::hash<dxfcpp::TimeSeriesTxModel<E>> {
    std::size_t operator()(const dxfcpp::TimeSeriesTxModel<E> &m) const noexcept {
        return m.hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()