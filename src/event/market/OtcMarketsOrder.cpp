// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OtcMarketsOrder.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &OtcMarketsOrder::TYPE = EventTypeEnum::OTC_MARKETS_ORDER;

void OtcMarketsOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    otcMarketsOrderData_ = {
        .quoteAccessPayment = graalOtcMarketsOrder->quote_access_payment,
        .otcMarketsFlags = graalOtcMarketsOrder->otc_markets_flags,
    };
}

void OtcMarketsOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillGraalData(graalNative);

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    graalOtcMarketsOrder->order_base.order_base.market_event.event_type.clazz = DXFG_EVENT_OTC_MARKETS_ORDER;
    graalOtcMarketsOrder->quote_access_payment = otcMarketsOrderData_.quoteAccessPayment;
    graalOtcMarketsOrder->otc_markets_flags = otcMarketsOrderData_.otcMarketsFlags;
}

OtcMarketsOrder::Ptr OtcMarketsOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create OtcMarketsOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to create OtcMarketsOrder. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_OTC_MARKETS_ORDER)));
    }

    auto otcMarketsOrder = std::make_shared<OtcMarketsOrder>();

    otcMarketsOrder->fillData(graalNative);

    return otcMarketsOrder;
}

void *OtcMarketsOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalOtcMarketsOrder = new dxfg_otc_markets_order_t{};

    fillGraalData(graalOtcMarketsOrder);

    return graalOtcMarketsOrder;
}

void OtcMarketsOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_OTC_MARKETS_ORDER) {
        throw InvalidArgumentException(
            fmt::format("Unable to free OtcMarketsOrder's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_OTC_MARKETS_ORDER)));
    }

    const auto graalOtcMarketsOrder = static_cast<dxfg_otc_markets_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalOtcMarketsOrder;
}

void OtcMarketsOrder::assign(std::shared_ptr<EventType> event) {
    Order::assign(event);

    if (const auto other = event->sharedAs<OtcMarketsOrder>(); other) {
        otcMarketsOrderData_ = other->otcMarketsOrderData_;
    }
}

OtcMarketsOrder::OtcMarketsOrder() noexcept {
}

OtcMarketsOrder::OtcMarketsOrder(const StringLike &eventSymbol) noexcept : Order(eventSymbol) {
}

OtcMarketsOrder &OtcMarketsOrder::withEventSymbol(const StringLike &eventSymbol) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withEventSymbol(eventSymbol));
}

OtcMarketsOrder &OtcMarketsOrder::withEventTime(std::int64_t eventTime) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withEventTime(eventTime));
}

OtcMarketsOrder &OtcMarketsOrder::withSource(const OrderSource &source) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withSource(source));
}

OtcMarketsOrder &OtcMarketsOrder::withEventFlags(std::int32_t eventFlags) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withEventFlags(eventFlags));
}

OtcMarketsOrder &OtcMarketsOrder::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withEventFlags(eventFlags));
}

OtcMarketsOrder &OtcMarketsOrder::withIndex(std::int64_t index) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withIndex(index));
}

OtcMarketsOrder &OtcMarketsOrder::withTime(std::int64_t time) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTime(time));
}

OtcMarketsOrder &OtcMarketsOrder::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTimeNanoPart(timeNanoPart));
}

OtcMarketsOrder &OtcMarketsOrder::withSequence(std::int32_t sequence) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withSequence(sequence));
}

OtcMarketsOrder &OtcMarketsOrder::withTimeNanos(std::int64_t timeNanos) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTimeNanos(timeNanos));
}

OtcMarketsOrder &OtcMarketsOrder::withAction(const OrderAction &action) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withAction(action));
}

OtcMarketsOrder &OtcMarketsOrder::withActionTime(std::int64_t actionTime) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withActionTime(actionTime));
}

OtcMarketsOrder &OtcMarketsOrder::withOrderId(std::int64_t orderId) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withOrderId(orderId));
}

OtcMarketsOrder &OtcMarketsOrder::withAuxOrderId(std::int64_t auxOrderId) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withAuxOrderId(auxOrderId));
}

OtcMarketsOrder &OtcMarketsOrder::withPrice(double price) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withPrice(price));
}

OtcMarketsOrder &OtcMarketsOrder::withSize(double size) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withSize(size));
}

OtcMarketsOrder &OtcMarketsOrder::withExecutedSize(double executedSize) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withExecutedSize(executedSize));
}

OtcMarketsOrder &OtcMarketsOrder::withCount(std::int64_t count) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withCount(count));
}

OtcMarketsOrder &OtcMarketsOrder::withTradeId(std::int64_t tradeId) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTradeId(tradeId));
}

OtcMarketsOrder &OtcMarketsOrder::withTradePrice(double tradePrice) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTradePrice(tradePrice));
}

OtcMarketsOrder &OtcMarketsOrder::withTradeSize(double tradeSize) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withTradeSize(tradeSize));
}

OtcMarketsOrder &OtcMarketsOrder::withExchangeCode(char exchangeCode) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withExchangeCode(exchangeCode));
}

OtcMarketsOrder &OtcMarketsOrder::withExchangeCode(std::int16_t exchangeCode) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withExchangeCode(exchangeCode));
}

OtcMarketsOrder &OtcMarketsOrder::withOrderSide(const Side &side) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withOrderSide(side));
}

OtcMarketsOrder &OtcMarketsOrder::withScope(const Scope &scope) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withScope(scope));
}

OtcMarketsOrder &OtcMarketsOrder::withMarketMaker(const StringLike &marketMaker) noexcept {
    return dynamic_cast<OtcMarketsOrder &>(Order::withMarketMaker(marketMaker));
}

std::int32_t OtcMarketsOrder::getQuoteAccessPayment() const noexcept {
    return otcMarketsOrderData_.quoteAccessPayment;
}

void OtcMarketsOrder::setQuoteAccessPayment(std::int32_t quoteAccessPayment) noexcept {
    otcMarketsOrderData_.quoteAccessPayment = quoteAccessPayment;
}

OtcMarketsOrder &OtcMarketsOrder::withQuoteAccessPayment(std::int32_t quoteAccessPayment) noexcept {
    setQuoteAccessPayment(quoteAccessPayment);

    return *this;
}

bool OtcMarketsOrder::isOpen() const noexcept {
    return andOp(otcMarketsOrderData_.otcMarketsFlags, OPEN) != 0;
}

void OtcMarketsOrder::setOpen(bool open) noexcept {
    if (open) {
        otcMarketsOrderData_.otcMarketsFlags = orOp(otcMarketsOrderData_.otcMarketsFlags, OPEN);
    } else {
        otcMarketsOrderData_.otcMarketsFlags = andOp(otcMarketsOrderData_.otcMarketsFlags, ~OPEN);
    }
}

OtcMarketsOrder &OtcMarketsOrder::withOpen(bool open) noexcept {
    setOpen(open);

    return *this;
}

bool OtcMarketsOrder::isUnsolicited() const noexcept {
    return andOp(otcMarketsOrderData_.otcMarketsFlags, UNSOLICITED) != 0;
}

void OtcMarketsOrder::setUnsolicited(bool unsolicited) noexcept {
    if (unsolicited) {
        otcMarketsOrderData_.otcMarketsFlags = orOp(otcMarketsOrderData_.otcMarketsFlags, UNSOLICITED);
    } else {
        otcMarketsOrderData_.otcMarketsFlags = andOp(otcMarketsOrderData_.otcMarketsFlags, ~UNSOLICITED);
    }
}

OtcMarketsOrder &OtcMarketsOrder::withUnsolicited(bool unsolicited) noexcept {
    setUnsolicited(unsolicited);

    return *this;
}

const OtcMarketsPriceType &OtcMarketsOrder::getOtcMarketsPriceType() const & noexcept {
    return OtcMarketsPriceType::valueOf(
        getBits(otcMarketsOrderData_.otcMarketsFlags, OTC_PRICE_TYPE_MASK, OTC_PRICE_TYPE_SHIFT));
}

void OtcMarketsOrder::setOtcMarketsPriceType(const OtcMarketsPriceType &otcPriceType) noexcept {
    otcMarketsOrderData_.otcMarketsFlags = setBits(otcMarketsOrderData_.otcMarketsFlags, OTC_PRICE_TYPE_MASK,
                                                   OTC_PRICE_TYPE_SHIFT, otcPriceType.getCode());
}

OtcMarketsOrder &OtcMarketsOrder::withOtcMarketsPriceType(const OtcMarketsPriceType &otcPriceType) noexcept {
    setOtcMarketsPriceType(otcPriceType);

    return *this;
}

bool OtcMarketsOrder::isSaturated() const noexcept {
    return andOp(otcMarketsOrderData_.otcMarketsFlags, SATURATED) != 0;
}

void OtcMarketsOrder::setSaturated(bool saturated) noexcept {
    if (saturated) {
        otcMarketsOrderData_.otcMarketsFlags = orOp(otcMarketsOrderData_.otcMarketsFlags, SATURATED);
    } else {
        otcMarketsOrderData_.otcMarketsFlags = andOp(otcMarketsOrderData_.otcMarketsFlags, ~SATURATED);
    }
}

OtcMarketsOrder &OtcMarketsOrder::withSaturated(bool saturated) noexcept {
    setSaturated(saturated);

    return *this;
}

bool OtcMarketsOrder::isAutoExecution() const noexcept {
    return andOp(otcMarketsOrderData_.otcMarketsFlags, AUTO_EXECUTION) != 0;
}

void OtcMarketsOrder::setAutoExecution(bool autoExecution) noexcept {
    if (autoExecution) {
        otcMarketsOrderData_.otcMarketsFlags = orOp(otcMarketsOrderData_.otcMarketsFlags, AUTO_EXECUTION);
    } else {
        otcMarketsOrderData_.otcMarketsFlags = andOp(otcMarketsOrderData_.otcMarketsFlags, ~AUTO_EXECUTION);
    }
}

OtcMarketsOrder &OtcMarketsOrder::withAutoExecution(bool autoExecution) noexcept {
    setAutoExecution(autoExecution);

    return *this;
}

bool OtcMarketsOrder::isNmsConditional() const noexcept {
    return andOp(otcMarketsOrderData_.otcMarketsFlags, NMS_CONDITIONAL) != 0;
}

void OtcMarketsOrder::setNmsConditional(bool nmsConditional) noexcept {
    if (nmsConditional) {
        otcMarketsOrderData_.otcMarketsFlags = orOp(otcMarketsOrderData_.otcMarketsFlags, NMS_CONDITIONAL);
    } else {
        otcMarketsOrderData_.otcMarketsFlags = andOp(otcMarketsOrderData_.otcMarketsFlags, ~NMS_CONDITIONAL);
    }
}

OtcMarketsOrder &OtcMarketsOrder::withNmsConditional(bool nmsConditional) noexcept {
    setNmsConditional(nmsConditional);

    return *this;
}

std::string OtcMarketsOrder::toString() const {
    return fmt::format("OtcMarketsOrder{{{}, marketMaker={}, QAP={}, open={}, unsolicited={}, priceType={}, "
                       "saturated={}, autoEx={}, NMS={}}}",
                       baseFieldsToString(), getMarketMaker(), getQuoteAccessPayment(), isOpen(), isUnsolicited(),
                       getOtcMarketsPriceType().toString(), isSaturated(), isAutoExecution(), isNmsConditional());
}

DXFCPP_END_NAMESPACE