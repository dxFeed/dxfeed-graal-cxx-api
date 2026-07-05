// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OrderBase.hpp"

#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

void OrderBase::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalOrderBase = static_cast<dxfg_order_base_t *>(graalNative);

    orderBaseData_ = {
        .eventFlags = graalOrderBase->event_flags,
        .index = graalOrderBase->index,
        .timeSequence = graalOrderBase->time_sequence,
        .timeNanoPart = graalOrderBase->time_nano_part,
        .actionTime = graalOrderBase->action_time,
        .orderId = graalOrderBase->order_id,
        .auxOrderId = graalOrderBase->aux_order_id,
        .price = graalOrderBase->price,
        .size = graalOrderBase->size,
        .executedSize = graalOrderBase->executed_size,
        .count = graalOrderBase->count,
        .flags = graalOrderBase->flags,
        .tradeId = graalOrderBase->trade_id,
        .tradePrice = graalOrderBase->trade_price,
        .tradeSize = graalOrderBase->trade_size,
    };
}

void OrderBase::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    const auto graalOrderBase = static_cast<dxfg_order_base_t *>(graalNative);

    graalOrderBase->event_flags = orderBaseData_.eventFlags;
    graalOrderBase->index = orderBaseData_.index;
    graalOrderBase->time_sequence = orderBaseData_.timeSequence;
    graalOrderBase->time_nano_part = orderBaseData_.timeNanoPart;
    graalOrderBase->action_time = orderBaseData_.actionTime;
    graalOrderBase->order_id = orderBaseData_.orderId;
    graalOrderBase->aux_order_id = orderBaseData_.auxOrderId;
    graalOrderBase->price = orderBaseData_.price;
    graalOrderBase->size = orderBaseData_.size;
    graalOrderBase->executed_size = orderBaseData_.executedSize;
    graalOrderBase->count = orderBaseData_.count;
    graalOrderBase->flags = orderBaseData_.flags;
    graalOrderBase->trade_id = orderBaseData_.tradeId;
    graalOrderBase->trade_price = orderBaseData_.tradePrice;
    graalOrderBase->trade_size = orderBaseData_.tradeSize;
}

void OrderBase::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<OrderBase>(); other) {
        orderBaseData_ = other->orderBaseData_;
    }
}

OrderBase::OrderBase() noexcept {
}

OrderBase::OrderBase(const StringLike &eventSymbol) noexcept : MarketEvent(eventSymbol) {
}

const OrderSource &OrderBase::getSource() const & noexcept {
    auto sourceId = static_cast<std::int32_t>(sar(orderBaseData_.index, SPECIAL_SOURCE_ID_SHIFT));

    if (!OrderSource::isSpecialSourceId(sourceId)) {
        sourceId = static_cast<std::int32_t>(sar(orderBaseData_.index, NONSPECIAL_SOURCE_ID_SHIFT));
    }

    return OrderSource::valueOf(sourceId);
}

void OrderBase::setSource(const OrderSource &source) noexcept {
    const auto shift =
        OrderSource::isSpecialSourceId(source.id()) ? SPECIAL_SOURCE_ID_SHIFT : NONSPECIAL_SOURCE_ID_SHIFT;
    const std::int64_t mask =
        OrderSource::isSpecialSourceId(static_cast<std::int32_t>(sar(orderBaseData_.index, SPECIAL_SOURCE_ID_SHIFT)))
            ? ~sal(std::int64_t{-1}, SPECIAL_SOURCE_ID_SHIFT)
            : ~sal(std::int64_t{-1}, NONSPECIAL_SOURCE_ID_SHIFT);

    orderBaseData_.index = andOp(sal(static_cast<std::int64_t>(source.id()), shift), andOp(orderBaseData_.index, mask));
}

std::int32_t OrderBase::getEventFlags() const noexcept {
    return orderBaseData_.eventFlags;
}

EventFlagsMask OrderBase::getEventFlagsMask() const noexcept {
    return EventFlagsMask(orderBaseData_.eventFlags);
}

void OrderBase::setEventFlags(std::int32_t eventFlags) noexcept {
    orderBaseData_.eventFlags = eventFlags;
}

void OrderBase::setEventFlags(const EventFlagsMask &eventFlags) noexcept {
    orderBaseData_.eventFlags = static_cast<std::int32_t>(eventFlags.getMask());
}

void OrderBase::setIndex(std::int64_t index) {
    if (index < 0) {
        throw InvalidArgumentException("Negative index: " + std::to_string(index));
    }

    orderBaseData_.index = index;
}

std::int64_t OrderBase::getIndex() const noexcept {
    return orderBaseData_.index;
}

std::int64_t OrderBase::getTimeSequence() const noexcept {
    return orderBaseData_.timeSequence;
}

void OrderBase::setTimeSequence(std::int64_t timeSequence) noexcept {
    orderBaseData_.timeSequence = timeSequence;
}

std::int64_t OrderBase::getTime() const noexcept {
    return sar(orderBaseData_.timeSequence, SECONDS_SHIFT) * 1000 +
           andOp(sar(orderBaseData_.timeSequence, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void OrderBase::setTime(std::int64_t time) noexcept {
    orderBaseData_.timeSequence =
        orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                  sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
             getSequence());
}

void OrderBase::setTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    orderBaseData_.timeNanoPart = timeNanoPart;
}

std::int32_t OrderBase::getTimeNanoPart() const noexcept {
    return orderBaseData_.timeNanoPart;
}

std::int32_t OrderBase::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(orderBaseData_.timeSequence, MAX_SEQUENCE));
}

void OrderBase::setSequence(std::int32_t sequence) {
    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    orderBaseData_.timeSequence = orOp(andOp(orderBaseData_.timeSequence, ~MAX_SEQUENCE), sequence);
}

std::int64_t OrderBase::getTimeNanos() const noexcept {
    return time_nanos_util::getNanosFromMillisAndNanoPart(getTime(), orderBaseData_.timeNanoPart);
}

void OrderBase::setTimeNanos(std::int64_t timeNanos) noexcept {
    setTime(time_nanos_util::getMillisFromNanos(timeNanos));
    orderBaseData_.timeNanoPart = time_nanos_util::getNanoPartFromNanos(timeNanos);
}

const OrderAction &OrderBase::getAction() const & noexcept {
    return OrderAction::valueOf(getBits(orderBaseData_.flags, ACTION_MASK, ACTION_SHIFT));
}

void OrderBase::setAction(const OrderAction &action) noexcept {
    orderBaseData_.flags = setBits(orderBaseData_.flags, ACTION_MASK, ACTION_SHIFT, action.getCode());
}

std::int64_t OrderBase::getActionTime() const noexcept {
    return orderBaseData_.actionTime;
}

void OrderBase::setActionTime(std::int64_t actionTime) noexcept {
    orderBaseData_.actionTime = actionTime;
}

std::int64_t OrderBase::getOrderId() const noexcept {
    return orderBaseData_.orderId;
}

void OrderBase::setOrderId(std::int64_t orderId) noexcept {
    orderBaseData_.orderId = orderId;
}

std::int64_t OrderBase::getAuxOrderId() const noexcept {
    return orderBaseData_.auxOrderId;
}

void OrderBase::setAuxOrderId(std::int64_t auxOrderId) noexcept {
    orderBaseData_.auxOrderId = auxOrderId;
}

double OrderBase::getPrice() const noexcept {
    return orderBaseData_.price;
}

void OrderBase::setPrice(double price) noexcept {
    orderBaseData_.price = price;
}

double OrderBase::getSize() const noexcept {
    return orderBaseData_.size;
}

void OrderBase::setSize(double size) noexcept {
    orderBaseData_.size = size;
}

bool OrderBase::hasSize() const noexcept {
    return orderBaseData_.size != 0 && !std::isnan(orderBaseData_.size);
}

double OrderBase::getExecutedSize() const noexcept {
    return orderBaseData_.executedSize;
}

void OrderBase::setExecutedSize(double executedSize) noexcept {
    orderBaseData_.executedSize = executedSize;
}

std::int64_t OrderBase::getCount() const noexcept {
    return orderBaseData_.count;
}

void OrderBase::setCount(std::int64_t count) noexcept {
    orderBaseData_.count = count;
}

std::int64_t OrderBase::getTradeId() const noexcept {
    return orderBaseData_.tradeId;
}

void OrderBase::setTradeId(std::int64_t tradeId) noexcept {
    orderBaseData_.tradeId = tradeId;
}

double OrderBase::getTradePrice() const noexcept {
    return orderBaseData_.tradePrice;
}

void OrderBase::setTradePrice(double tradePrice) noexcept {
    orderBaseData_.tradePrice = tradePrice;
}

double OrderBase::getTradeSize() const noexcept {
    return orderBaseData_.tradeSize;
}

void OrderBase::setTradeSize(double tradeSize) noexcept {
    orderBaseData_.tradeSize = tradeSize;
}

std::int16_t OrderBase::getExchangeCode() const noexcept {
    return utf8to16(
        static_cast<char>(static_cast<unsigned char>(getBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT))));
}

std::string OrderBase::getExchangeCodeString() const noexcept {
    // TODO: cache [EN-8231]

    return std::string( // NOLINT(*-return-braced-init-list)
        1ULL,
        static_cast<char>(static_cast<unsigned char>(getBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT))));
}

void OrderBase::setExchangeCode(char exchangeCode) {
    util::checkChar(exchangeCode, EXCHANGE_MASK, "exchangeCode");

    orderBaseData_.flags =
        setBits(orderBaseData_.flags, EXCHANGE_MASK, EXCHANGE_SHIFT, static_cast<unsigned char>(exchangeCode));
}

void OrderBase::setExchangeCode(std::int16_t exchangeCode) noexcept {
    setExchangeCode(utf16to8(exchangeCode));
}

const Side &OrderBase::getOrderSide() const & noexcept {
    return Side::valueOf(getBits(orderBaseData_.flags, SIDE_MASK, SIDE_SHIFT));
}

void OrderBase::setOrderSide(const Side &side) noexcept {
    orderBaseData_.flags = setBits(orderBaseData_.flags, SIDE_MASK, SIDE_SHIFT, side.getCode());
}

const Scope &OrderBase::getScope() const & noexcept {
    return Scope::valueOf(getBits(orderBaseData_.flags, SCOPE_MASK, SCOPE_SHIFT));
}

void OrderBase::setScope(const Scope &scope) noexcept {
    orderBaseData_.flags = setBits(orderBaseData_.flags, SCOPE_MASK, SCOPE_SHIFT, scope.getCode());
}

std::string OrderBase::baseFieldsToString() const {
    return fmt::format(
        "{}, eventTime={}, source={}, eventFlags={:#x}, index={:#x}, time={}, sequence={}, "
        "timeNanoPart={}, action={}, actionTime={}, orderId={}, auxOrderId={}, price={}, "
        "size={}, executedSize={}, count={}, exchange={}, side={}, scope={}, tradeId={}, tradePrice={}, tradeSize={}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getSource().toString(), getEventFlagsMask().getMask(), getIndex(),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(), getTimeNanoPart(), getAction().toString(),
        TimeFormat::DEFAULT_WITH_MILLIS.format(getActionTime()), getOrderId(), getAuxOrderId(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getSize()), dxfcpp::toString(getExecutedSize()), getCount(),
        encodeChar(getExchangeCode()), getOrderSide().toString(), getScope().toString(), getTradeId(),
        dxfcpp::toString(getTradePrice()), dxfcpp::toString(getTradeSize()));
}

DXFCPP_END_NAMESPACE