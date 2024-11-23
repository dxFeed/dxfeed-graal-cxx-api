// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../symbols/SymbolWrapper.hpp"
#include "../entity/SharedEntity.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "TxModelListener.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;

struct DXFCPP_EXPORT TimeSeriesTxModel : RequireMakeShared<TimeSeriesTxModel> {

    struct DXFCPP_EXPORT Builder : RequireMakeShared<Builder> {
    private:
        JavaObjectHandle<Builder> handle_;
        std::shared_ptr<TxModelListenerCommon> listener_;

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
         * auto listener = TxModelListener<Candle>::create([](const auto &, const auto &events, bool isSnapshot) {
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
        template <Derived<TimeSeriesEvent> E>
        std::shared_ptr<Builder> withListener(std::shared_ptr<TxModelListener<E>> listener) const {
            return createShared(std::move(withListenerImpl(listener->getHandle())),
                                listener->template sharedAs<TxModelListenerCommon>());
        }
    };

private:
    JavaObjectHandle<IndexedTxModel> handle_;
    std::shared_ptr<TxModelListenerCommon> listener_;

public:
    TimeSeriesTxModel(LockExternalConstructionTag tag, JavaObjectHandle<TimeSeriesTxModel> &&handle);

    TimeSeriesTxModel(LockExternalConstructionTag tag, JavaObjectHandle<TimeSeriesTxModel> &&handle,
                   std::shared_ptr<TxModelListenerCommon> listener);
};

DXFCPP_END_NAMESPACE