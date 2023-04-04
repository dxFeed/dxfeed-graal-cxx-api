// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8cpp/utf8.h>
#include <utility>

namespace dxfcpp {

const EventTypeEnum &TradeETH::Type = EventTypeEnum::TRADE_ETH;

std::shared_ptr<TradeETH> TradeETH::fromGraalNative(void *graalNative) noexcept {
    return TradeBase::fromGraalNative<TradeETH, dxfg_event_type_t, dxfg_trade_eth_t,
                                      dxfg_event_clazz_t::DXFG_EVENT_TRADE_ETH>(graalNative);
}

} // namespace dxfcpp