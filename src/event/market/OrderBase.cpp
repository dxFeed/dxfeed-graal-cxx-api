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
    MarketEvent::fillData(graalNative);

    auto graalOrderBase = bit_cast<dxfg_order_base_t *>(graalNative);

    orderBaseData_ = {
        graalOrderBase->event_flags,    graalOrderBase->index,       graalOrderBase->time_sequence,
        graalOrderBase->time_nano_part, graalOrderBase->action_time, graalOrderBase->order_id,
        graalOrderBase->aux_order_id,   graalOrderBase->price,       graalOrderBase->size,
        graalOrderBase->executed_size,  graalOrderBase->count,       graalOrderBase->flags,
        graalOrderBase->trade_id,       graalOrderBase->trade_price, graalOrderBase->trade_size,
    };
}

std::string OrderBase::baseFieldsToString() const noexcept {
    return fmt::format(
        "{}, eventTime={}, source={}, eventFlags={:#x}, index={:#x}, time={}, sequence={}, "
        "timeNanoPart={}, action={}, actionTime={}, orderId={}, auxOrderId={}, price={}, "
        "size={}, executedSize={}, count={}, exchange={}, side={}, scope={}, tradeId={}, tradePrice={}, tradeSize={}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()), getSource().toString(),
        getEventFlags().getMask(), getIndex(), formatTimeStampWithMillis(getTime()), getSequence(), getTimeNanoPart(),
        getAction().toString(), formatTimeStampWithMillis(getActionTime()), getOrderId(), getAuxOrderId(),
        dxfcpp::toString(getPrice()), dxfcpp::toString(getSize()), dxfcpp::toString(getExecutedSize()), getCount(),
        encodeChar(getExchangeCode()), getOrderSide().toString(), getScope().toString(), getTradeId(),
        dxfcpp::toString(getTradePrice()), dxfcpp::toString(getTradeSize()));
}

} // namespace dxfcpp