// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/SpreadOrder.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &SpreadOrder::TYPE = EventTypeEnum::SPREAD_ORDER;

void SpreadOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    spreadOrderData_ = {.spreadSymbol = toStringOpt(graalSpreadOrder->spread_symbol)};
}

void SpreadOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    OrderBase::fillGraalData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    graalSpreadOrder->order_base.market_event.event_type.clazz = DXFG_EVENT_SPREAD_ORDER;
    graalSpreadOrder->spread_symbol = createCString(spreadOrderData_.spreadSymbol);
}

void SpreadOrder::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::freeGraalData(graalNative);

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    delete[] graalSpreadOrder->spread_symbol;
}

std::shared_ptr<SpreadOrder> SpreadOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create SpreadOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        throw InvalidArgumentException(fmt::format("Unable to create SpreadOrder. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_SPREAD_ORDER)));
    }

    auto spreadOrder = std::make_shared<SpreadOrder>();

    spreadOrder->fillData(graalNative);

    return spreadOrder;
}

void *SpreadOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSpreadOrder = new dxfg_spread_order_t{};

    fillGraalData(graalSpreadOrder);

    return graalSpreadOrder;
}

void SpreadOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SPREAD_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to free SpreadOrder's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_SPREAD_ORDER)));
    }

    const auto graalSpreadOrder = static_cast<dxfg_spread_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalSpreadOrder;
}

void SpreadOrder::assign(std::shared_ptr<EventType> event) {
    OrderBase::assign(event);

    if (const auto other = event->sharedAs<SpreadOrder>(); other) {
        spreadOrderData_ = other->spreadOrderData_;
    }
}

SpreadOrder::SpreadOrder() noexcept {
}

SpreadOrder::SpreadOrder(const StringLike &eventSymbol) noexcept : OrderBase(eventSymbol) {
}

SpreadOrder &SpreadOrder::withEventSymbol(const StringLike &eventSymbol) noexcept {
    MarketEvent::setEventSymbol(eventSymbol);

    return *this;
}

SpreadOrder &SpreadOrder::withEventTime(std::int64_t eventTime) noexcept {
    MarketEvent::setEventTime(eventTime);

    return *this;
}

SpreadOrder &SpreadOrder::withSource(const OrderSource &source) noexcept {
    setSource(source);

    return *this;
}

SpreadOrder &SpreadOrder::withEventFlags(std::int32_t eventFlags) noexcept {
    OrderBase::setEventFlags(eventFlags);

    return *this;
}

SpreadOrder &SpreadOrder::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    OrderBase::setEventFlags(eventFlags);

    return *this;
}

SpreadOrder &SpreadOrder::withIndex(std::int64_t index) noexcept {
    OrderBase::setIndex(index);

    return *this;
}

SpreadOrder &SpreadOrder::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

SpreadOrder &SpreadOrder::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    setTimeNanoPart(timeNanoPart);

    return *this;
}

SpreadOrder &SpreadOrder::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

SpreadOrder &SpreadOrder::withTimeNanos(std::int64_t timeNanos) noexcept {
    setTimeNanos(timeNanos);

    return *this;
}

SpreadOrder &SpreadOrder::withAction(const OrderAction &action) noexcept {
    setAction(action);

    return *this;
}

SpreadOrder &SpreadOrder::withActionTime(std::int64_t actionTime) noexcept {
    setActionTime(actionTime);

    return *this;
}

SpreadOrder &SpreadOrder::withOrderId(std::int64_t orderId) noexcept {
    setOrderId(orderId);

    return *this;
}

SpreadOrder &SpreadOrder::withAuxOrderId(std::int64_t auxOrderId) noexcept {
    setAuxOrderId(auxOrderId);

    return *this;
}

SpreadOrder &SpreadOrder::withPrice(double price) noexcept {
    setPrice(price);

    return *this;
}

SpreadOrder &SpreadOrder::withSize(double size) noexcept {
    setSize(size);

    return *this;
}

SpreadOrder &SpreadOrder::withExecutedSize(double executedSize) noexcept {
    setExecutedSize(executedSize);

    return *this;
}

SpreadOrder &SpreadOrder::withCount(std::int64_t count) noexcept {
    setCount(count);

    return *this;
}

SpreadOrder &SpreadOrder::withTradeId(std::int64_t tradeId) noexcept {
    setTradeId(tradeId);

    return *this;
}

SpreadOrder &SpreadOrder::withTradePrice(double tradePrice) noexcept {
    setTradePrice(tradePrice);

    return *this;
}

SpreadOrder &SpreadOrder::withTradeSize(double tradeSize) noexcept {
    setTradeSize(tradeSize);

    return *this;
}

SpreadOrder &SpreadOrder::withExchangeCode(char exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

SpreadOrder &SpreadOrder::withExchangeCode(std::int16_t exchangeCode) noexcept {
    setExchangeCode(exchangeCode);

    return *this;
}

SpreadOrder &SpreadOrder::withOrderSide(const Side &side) noexcept {
    setOrderSide(side);

    return *this;
}

SpreadOrder &SpreadOrder::withScope(const Scope &scope) noexcept {
    setScope(scope);

    return *this;
}

const std::string &SpreadOrder::getSpreadSymbol() const & noexcept {
    if (!spreadOrderData_.spreadSymbol) {
        return String::NUL;
    }

    return spreadOrderData_.spreadSymbol.value();
}

const std::optional<std::string> &SpreadOrder::getSpreadSymbolOpt() const & noexcept {
    return spreadOrderData_.spreadSymbol;
}

void SpreadOrder::setSpreadSymbol(const StringLike &spreadSymbol) noexcept {
    spreadOrderData_.spreadSymbol = std::string(spreadSymbol);
}

SpreadOrder &SpreadOrder::withSpreadSymbol(const StringLike &spreadSymbol) noexcept {
    setSpreadSymbol(spreadSymbol);

    return *this;
}

std::string SpreadOrder::toString() const {
    return fmt::format("SpreadOrder{{{}, spreadSymbol={}}}", baseFieldsToString(), getSpreadSymbol());
}

DXFCPP_END_NAMESPACE