// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/EventTypeEnum.hpp>
#include <dxfeed_graal_cpp_api/event/market/AnalyticOrder.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>
#include <dxfeed_graal_cpp_api/internal/utils/debug/Debug.hpp>
#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &AnalyticOrder::TYPE = EventTypeEnum::ANALYTIC_ORDER;

void AnalyticOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    const auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    analyticOrderData_ = {.icebergPeakSize = graalAnalyticOrder->iceberg_peak_size,
                          .icebergHiddenSize = graalAnalyticOrder->iceberg_hidden_size,
                          .icebergExecutedSize = graalAnalyticOrder->iceberg_executed_size,
                          .icebergFlags = graalAnalyticOrder->iceberg_flags};
}

void AnalyticOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillGraalData(graalNative);

    const auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    graalAnalyticOrder->order_base.order_base.market_event.event_type.clazz = DXFG_EVENT_ANALYTIC_ORDER;
    graalAnalyticOrder->iceberg_peak_size = analyticOrderData_.icebergPeakSize;
    graalAnalyticOrder->iceberg_hidden_size = analyticOrderData_.icebergHiddenSize;
    graalAnalyticOrder->iceberg_executed_size = analyticOrderData_.icebergExecutedSize;
    graalAnalyticOrder->iceberg_flags = analyticOrderData_.icebergFlags;
}

std::shared_ptr<AnalyticOrder> AnalyticOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create AnalyticOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ANALYTIC_ORDER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create AnalyticOrder. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_ANALYTIC_ORDER)));
    }

    auto analyticOrder = std::make_shared<AnalyticOrder>();

    analyticOrder->fillData(graalNative);

    return analyticOrder;
}

void *AnalyticOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalAnalyticOrder = new dxfg_analytic_order_t{};

    fillGraalData(graalAnalyticOrder);

    return graalAnalyticOrder;
}

void AnalyticOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ANALYTIC_ORDER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free AnalyticOrder's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_ANALYTIC_ORDER)));
    }

    const auto graalAnalyticOrder = static_cast<dxfg_analytic_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalAnalyticOrder;
}

void AnalyticOrder::assign(std::shared_ptr<EventType> event) {
    Order::assign(event);

    if (const auto other = event->sharedAs<AnalyticOrder>(); other) {
        analyticOrderData_ = other->analyticOrderData_;
    }
}

AnalyticOrder::AnalyticOrder(const StringLike &eventSymbol) noexcept : Order(eventSymbol) {
}

AnalyticOrder &AnalyticOrder::withEventSymbol(const StringLike &eventSymbol) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withEventSymbol(eventSymbol));
}

AnalyticOrder &AnalyticOrder::withEventTime(std::int64_t eventTime) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withEventTime(eventTime));
}

AnalyticOrder &AnalyticOrder::withSource(const OrderSource &source) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withSource(source));
}

AnalyticOrder &AnalyticOrder::withEventFlags(std::int32_t eventFlags) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withEventFlags(eventFlags));
}

AnalyticOrder &AnalyticOrder::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withEventFlags(eventFlags));
}

AnalyticOrder &AnalyticOrder::withIndex(std::int64_t index) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withIndex(index));
}

AnalyticOrder &AnalyticOrder::withTime(std::int64_t time) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTime(time));
}

AnalyticOrder &AnalyticOrder::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTimeNanoPart(timeNanoPart));
}

AnalyticOrder &AnalyticOrder::withSequence(std::int32_t sequence) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withSequence(sequence));
}

AnalyticOrder &AnalyticOrder::withTimeNanos(std::int64_t timeNanos) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTimeNanos(timeNanos));
}

AnalyticOrder &AnalyticOrder::withAction(const OrderAction &action) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withAction(action));
}

AnalyticOrder &AnalyticOrder::withActionTime(std::int64_t actionTime) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withActionTime(actionTime));
}

AnalyticOrder &AnalyticOrder::withOrderId(std::int64_t orderId) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withOrderId(orderId));
}

AnalyticOrder &AnalyticOrder::withAuxOrderId(std::int64_t auxOrderId) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withAuxOrderId(auxOrderId));
}

AnalyticOrder &AnalyticOrder::withPrice(double price) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withPrice(price));
}

AnalyticOrder &AnalyticOrder::withSize(double size) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withSize(size));
}

AnalyticOrder &AnalyticOrder::withExecutedSize(double executedSize) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withExecutedSize(executedSize));
}

AnalyticOrder &AnalyticOrder::withCount(std::int64_t count) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withCount(count));
}

AnalyticOrder &AnalyticOrder::withTradeId(std::int64_t tradeId) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTradeId(tradeId));
}

AnalyticOrder &AnalyticOrder::withTradePrice(double tradePrice) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTradePrice(tradePrice));
}

AnalyticOrder &AnalyticOrder::withTradeSize(double tradeSize) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withTradeSize(tradeSize));
}

AnalyticOrder &AnalyticOrder::withExchangeCode(char exchangeCode) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withExchangeCode(exchangeCode));
}

AnalyticOrder &AnalyticOrder::withExchangeCode(std::int16_t exchangeCode) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withExchangeCode(exchangeCode));
}

AnalyticOrder &AnalyticOrder::withOrderSide(const Side &side) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withOrderSide(side));
}

AnalyticOrder &AnalyticOrder::withScope(const Scope &scope) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withScope(scope));
}

AnalyticOrder &AnalyticOrder::withMarketMaker(const StringLike &marketMaker) noexcept {
    return dynamic_cast<AnalyticOrder &>(Order::withMarketMaker(marketMaker));
}

double AnalyticOrder::getIcebergPeakSize() const noexcept {
    return analyticOrderData_.icebergPeakSize;
}

void AnalyticOrder::setIcebergPeakSize(double icebergPeakSize) noexcept {
    analyticOrderData_.icebergPeakSize = icebergPeakSize;
}

AnalyticOrder &AnalyticOrder::withIcebergPeakSize(double icebergPeakSize) noexcept {
    setIcebergPeakSize(icebergPeakSize);

    return *this;
}

double AnalyticOrder::getIcebergHiddenSize() const noexcept {
    return analyticOrderData_.icebergHiddenSize;
}

void AnalyticOrder::setIcebergHiddenSize(double icebergHiddenSize) noexcept {
    analyticOrderData_.icebergHiddenSize = icebergHiddenSize;
}

AnalyticOrder &AnalyticOrder::withIcebergHiddenSize(double icebergHiddenSize) noexcept {
    setIcebergHiddenSize(icebergHiddenSize);

    return *this;
}

double AnalyticOrder::getIcebergExecutedSize() const noexcept {
    return analyticOrderData_.icebergExecutedSize;
}

void AnalyticOrder::setIcebergExecutedSize(double icebergExecutedSize) noexcept {
    analyticOrderData_.icebergExecutedSize = icebergExecutedSize;
}

AnalyticOrder &AnalyticOrder::withIcebergExecutedSize(double icebergExecutedSize) noexcept {
    setIcebergExecutedSize(icebergExecutedSize);

    return *this;
}

const IcebergType &AnalyticOrder::getIcebergType() const & noexcept {
    return IcebergType::valueOf(getBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT));
}

void AnalyticOrder::setIcebergType(const IcebergType &icebergType) noexcept {
    analyticOrderData_.icebergFlags =
        setBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT, icebergType.getCode());
}

AnalyticOrder &AnalyticOrder::withIcebergType(const IcebergType &icebergType) noexcept {
    setIcebergType(icebergType);

    return *this;
}

std::string AnalyticOrder::toString() const {
    return fmt::format("AnalyticOrder{{{}, marketMaker={}, icebergPeakSize={}, icebergHiddenSize={}, "
                       "icebergExecutedSize={}, icebergType={}}}",
                       baseFieldsToString(), getMarketMaker(), dxfcpp::toString(getIcebergPeakSize()),
                       dxfcpp::toString(getIcebergHiddenSize()), dxfcpp::toString(getIcebergExecutedSize()),
                       getIcebergType().toString());
}

DXFCPP_END_NAMESPACE