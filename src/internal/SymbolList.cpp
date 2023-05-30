// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

std::string toString(const dxfg_symbol_list &graalSymbolList);

// TODO: CandleSymbol, indexed_event_subscription_symbol, time_series_subscription_symbol,
// ::toGraal
struct SymbolList::Impl : public RawListWrapper<dxfg_symbol_list, [](dxfg_symbol_list &list, std::size_t index,
                                                                     const SymbolWrapper &symbolWrapper) {
    *list.elements[index] = *bit_cast<dxfg_symbol_t *>(symbolWrapper.toGraal());
}> {
    std::string toString() const noexcept { return "SymbolList::Impl{list = " + dxfcpp::toString(list_) + "}"; }
};

SymbolList::SymbolList() noexcept : impl_(std::make_unique<SymbolList::Impl>()) {}

std::unique_ptr<SymbolList> SymbolList::create(std::size_t size) noexcept {
    auto result = std::unique_ptr<SymbolList>(new SymbolList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

void SymbolList::set(std::size_t index, const SymbolWrapper &symbolWrapper) noexcept {
    impl_->set(index, symbolWrapper);
}

bool SymbolList::isEmpty() const noexcept { return impl_->isEmpty(); }

std::size_t SymbolList::size() const noexcept { return impl_->size(); }

void *SymbolList::getHandler() noexcept { return impl_->getHandler(); }

SymbolList::~SymbolList() noexcept = default;

std::string SymbolList::toString() const noexcept { return "SymbolList{impl = " + impl_->toString() + "}"; }

std::string toString(const dxfg_indexed_event_source_t &graalEventSource) {
    std::string result = "dxfg_indexed_event_source_t{";

    switch (graalEventSource.type) {

    case INDEXED_EVENT_SOURCE:
        result += "type = INDEXED_EVENT_SOURCE";

        break;

    case ORDER_SOURCE:
        result += "type = ORDER_SOURCE";

        break;
    }

    result += ", id = " + toStringAny(graalEventSource.id);
    result += std::string(", name = ") + (graalEventSource.name ? "null" : graalEventSource.name);

    return result;
}

std::string toString(const dxfg_symbol_t *graalSymbol) {
    std::string result = "dxfg_symbol_t{type = ";
    switch (graalSymbol->type) {

    case STRING: {
        result += "STRING";

        const auto *stringSymbol = bit_cast<const dxfg_string_symbol_t *>(graalSymbol);

        result += std::string(", symbol = ") + (!stringSymbol->symbol ? "null" : stringSymbol->symbol);

        break;
    }
    case CANDLE: {
        result += "CANDLE";

        const auto *candleSymbol = bit_cast<const dxfg_candle_symbol_t *>(graalSymbol);

        result += std::string(", symbol = ") + (!candleSymbol->symbol ? "null" : candleSymbol->symbol);

        break;
    }
    case WILDCARD:
        result += "WILDCARD";
        break;
    case INDEXED_EVENT_SUBSCRIPTION: {
        result += "INDEXED_EVENT_SUBSCRIPTION";

        const auto *indexedEventSubscriptionSymbol =
            bit_cast<const dxfg_indexed_event_subscription_symbol_t *>(graalSymbol);

        auto symbol =
            !indexedEventSubscriptionSymbol->symbol ? "null" : toString(indexedEventSubscriptionSymbol->symbol);

        result += ",symbol = " + symbol;
        result +=
            ",source = " +
            (!indexedEventSubscriptionSymbol->source ? "null" : toString(*indexedEventSubscriptionSymbol->source));

        break;
    }
    case TIME_SERIES_SUBSCRIPTION: {
        result += "TIME_SERIES_SUBSCRIPTION";

        const auto *timeSeriesSubscriptionSymbol =
            bit_cast<const dxfg_time_series_subscription_symbol_t *>(graalSymbol);

        auto symbol = !timeSeriesSubscriptionSymbol->symbol ? "null" : toString(timeSeriesSubscriptionSymbol->symbol);

        result += ",symbol = " + symbol;
        result += ",from_time = " + formatTimeStampWithMillis(timeSeriesSubscriptionSymbol->from_time);

        break;
    }
    default:
        result += "UNKNOWN(" + std::to_string(graalSymbol->type) + ")";
    }

    result += "}";

    return result;
}

std::string toString(const dxfg_symbol_list &graalSymbolList) {
    std::string result = "dxfg_symbol_list{";

    result += "size = " + std::to_string(graalSymbolList.size);
    result += ", elements = ";

    if (!graalSymbolList.elements) {
        result += "null";
    } else {
        result += "[";
        for (auto i = 0; i < graalSymbolList.size; i++) {
            result += !graalSymbolList.elements[i] ? "null" : toString(graalSymbolList.elements[i]) + ", ";
        }

        result += "]";
    }

    result += "}";

    return result;
}

} // namespace dxfcpp