// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "../internal/JavaObjectHandle.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../event/market/OrderSource.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFeed;

struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    struct DXFCPP_EXPORT Builder {};

private:
    JavaObjectHandle<IndexedTxModel> handle_;

    void setSourcesImpl(void *graalEventSourceList) const;
public:
    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum& eventType);

    bool isBatchProcessing() const;

    bool isSnapshotProcessing() const;

    void attach(std::shared_ptr<DXFeed> feed) const;

    void detach(std::shared_ptr<DXFeed> feed) const;

    void close() const;

    std::unordered_set<EventSourceWrapper> getSources() const;

    template <typename EventSourceIt>
    void setSources(EventSourceIt begin, EventSourceIt end) const {
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