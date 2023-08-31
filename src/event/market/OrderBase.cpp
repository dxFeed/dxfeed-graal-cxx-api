// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void OrderBase::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalOrderBase = static_cast<dxfg_order_base_t *>(graalNative);

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

    auto graalOrderBase = static_cast<dxfg_order_base_t *>(graalNative);

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

std::string OrderBase::baseFieldsToString() const noexcept {
    return fmt::format(
        "{}, eventTime={}, source={}, eventFlags={:#x}, index={:#x}, time={}, sequence={}, "
        "timeNanoPart={}, action={}, actionTime={}, orderId={}, auxOrderId={}, price={}, "
        "size={}, executedSize={}, count={}, exchange={}, side={}, scope={}, tradeId={}, tradePrice={}, tradeSize={}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()), getSource().toString(),
        getEventFlagsMask().getMask(), getIndex(), formatTimeStampWithMillis(getTime()), getSequence(),
        getTimeNanoPart(), getAction().toString(), formatTimeStampWithMillis(getActionTime()), getOrderId(),
        getAuxOrderId(), dxfcpp::toString(getPrice()), dxfcpp::toString(getSize()), dxfcpp::toString(getExecutedSize()),
        getCount(), encodeChar(getExchangeCode()), getOrderSide().toString(), getScope().toString(), getTradeId(),
        dxfcpp::toString(getTradePrice()), dxfcpp::toString(getTradeSize()));
}

} // namespace dxfcpp