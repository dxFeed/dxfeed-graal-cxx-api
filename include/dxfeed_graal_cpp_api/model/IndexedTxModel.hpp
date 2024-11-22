// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/market/OrderSource.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct SymbolWrapper;
struct DXFeed;
struct TxModelListener;

struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    struct DXFCPP_EXPORT Builder : RequireMakeShared<Builder> {
      private:
        JavaObjectHandle<Builder> handle_;
        std::shared_ptr<TxModelListener> listener_;

        JavaObjectHandle<Builder> withSourcesImpl(void *graalEventSourceList) const;

      public:
        Builder(LockExternalConstructionTag tag, JavaObjectHandle<Builder>&& handle);

        Builder(LockExternalConstructionTag tag, JavaObjectHandle<Builder>&& handle, std::shared_ptr<TxModelListener> listener);

        std::shared_ptr<Builder> withBatchProcessing(bool isBatchProcessing) const;

        std::shared_ptr<Builder> withSnapshotProcessing(bool isSnapshotProcessing) const;

        std::shared_ptr<Builder> withFeed(std::shared_ptr<DXFeed> feed) const;

        std::shared_ptr<Builder> withSymbol(const SymbolWrapper &symbol) const;

        std::shared_ptr<Builder> withListener(std::shared_ptr<TxModelListener> listener) const;

        template <typename EventSourceIt>
        std::shared_ptr<Builder> withSources(EventSourceIt begin, EventSourceIt end) const {
            auto list = EventSourceWrapper::ListUtils::toGraalListUnique(begin, end);

            return createShared(std::move(withSourcesImpl(list.get())));
        }

        template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
        std::shared_ptr<Builder> withSources(EventSourceCollection &&collection) const {
            return withSources(std::begin(collection), std::end(collection));
        }

        std::shared_ptr<Builder> withSources(std::initializer_list<EventSourceWrapper> collection) const;

        std::shared_ptr<IndexedTxModel> build() const;
    };

  private:
    JavaObjectHandle<IndexedTxModel> handle_;
    std::shared_ptr<TxModelListener> listener_;

    void setSourcesImpl(void *graalEventSourceList) const;

  public:
    IndexedTxModel(LockExternalConstructionTag tag, JavaObjectHandle<IndexedTxModel>&& handle);

    IndexedTxModel(LockExternalConstructionTag tag, JavaObjectHandle<IndexedTxModel>&& handle, std::shared_ptr<TxModelListener> listener);

    ~IndexedTxModel() noexcept override;

    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum &eventType);

    bool isBatchProcessing() const;

    bool isSnapshotProcessing() const;

    void attach(std::shared_ptr<DXFeed> feed) const;

    void detach(std::shared_ptr<DXFeed> feed) const;

    void close() const;

    std::unordered_set<EventSourceWrapper> getSources() const;

    template <typename EventSourceIt> void setSources(EventSourceIt begin, EventSourceIt end) const {
        auto list = EventSourceWrapper::ListUtils::toGraalListUnique(begin, end);

        setSourcesImpl(list.get());
    }

    template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
    void setSources(EventSourceCollection &&collection) const {
        setSources(std::begin(collection), std::end(collection));
    }

    void setSources(std::initializer_list<EventSourceWrapper> collection) const;
};

DXFCPP_END_NAMESPACE