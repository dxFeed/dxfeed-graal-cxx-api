// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../event/market/OrderSource.hpp"
#include "../symbols/SymbolWrapper.hpp"
#include "../entity/SharedEntity.hpp"

#include <memory>
#include <unordered_set>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_EXPORT EventSourceWrapper final {
    using DataType = std::variant<IndexedEventSource, OrderSource>;

private:
    DataType data_;

public:
    EventSourceWrapper(const EventSourceWrapper &) noexcept = default;
    EventSourceWrapper(EventSourceWrapper &&) noexcept = default;
    EventSourceWrapper &operator=(const EventSourceWrapper &) noexcept = default;
    EventSourceWrapper &operator=(EventSourceWrapper &&) noexcept = default;
    EventSourceWrapper() noexcept = default;
    ~EventSourceWrapper() noexcept = default;

    EventSourceWrapper(const IndexedEventSource &data) noexcept {
        data_ = data;
    }

    EventSourceWrapper(const OrderSource &data) noexcept {
        data_ = data;
    }

    static void freeGraal(void *graalNative) {
        IndexedEventSource::freeGraal(graalNative);
    }

    static EventSourceWrapper fromGraal(void *graalNative);

    void *toGraal() const noexcept {
        return std::visit(
            [](const auto &eventSource) {
                return eventSource.toGraal();
            },
            data_);
    }

    std::unique_ptr<void, decltype(&EventSourceWrapper::freeGraal)> toGraalUnique() const noexcept {
        return {toGraal(), freeGraal};
    }

    std::string toString() const {
        return "EventSourceWrapper{" +
               std::visit(
                   [](const auto &eventSource) {
                       return toStringAny(eventSource);
                   },
                   data_) +
               "}";
    }

    std::string toStringUnderlying() const {
        return std::visit(
            [](const auto &eventSource) {
                return toStringAny(eventSource);
            },
            data_);
    }

    bool isIndexedEventSource() const noexcept {
        return std::holds_alternative<IndexedEventSource>(data_);
    }

    bool isOrderSource() const noexcept {
        return std::holds_alternative<OrderSource>(data_);
    }

    std::optional<IndexedEventSource> asIndexedEventSource() const noexcept {
        return isIndexedEventSource() ? std::make_optional<IndexedEventSource>(std::get<IndexedEventSource>(data_)) : std::nullopt;
    }

    std::optional<OrderSource> asOrderSource() const noexcept {
        return isOrderSource() ? std::make_optional<OrderSource>(std::get<OrderSource>(data_)) : std::nullopt;
    }

    const DataType &getData() const noexcept {
        return data_;
    }

    bool operator==(const EventSourceWrapper &eventSourceWrapper) const {
        return getData() == eventSourceWrapper.getData();
    }

    auto operator<(const EventSourceWrapper &eventSourceWrapper) const {
        return getData() < eventSourceWrapper.getData();
    }

    using GraalPtr = std::unique_ptr<void, decltype(&EventSourceWrapper::freeGraal)>;
};

struct DXFCPP_EXPORT IndexedTxModel : RequireMakeShared<IndexedTxModel> {

    struct DXFCPP_EXPORT Builder {};

    static std::shared_ptr<Builder> newBuilder(const EventTypeEnum& eventType);


};

DXFCPP_END_NAMESPACE