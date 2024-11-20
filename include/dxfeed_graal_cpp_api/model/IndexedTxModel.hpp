// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../event/market/OrderSource.hpp"
#include "../event/EventSourceWrapper.hpp"
#include "../entity/SharedEntity.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    struct DXFCPP_EXPORT Builder {};

private:
    void setSourcesImpl(void *graalEventSourceList) const;
public:
    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum& eventType);

    std::unordered_set<EventSourceWrapper> getSources() const;

    template <typename EventSourceIt>
    void setSources(EventSourceIt begin, EventSourceIt end) const {
        auto list = EventSourceWrapper::ListUtils::toGraalListUnique(begin, end);

        setSymbolsImpl(list.get());
    }

    template <ConvertibleToEventSourceWrapperCollection EventSourceCollection>
    void setSources(EventSourceCollection &&collection) const;

    void setSources(std::initializer_list<EventSourceWrapper> collection) const;
};

DXFCPP_END_NAMESPACE