// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

const EventTypeEnum &Trade::Type = EventTypeEnum::TRADE;

std::shared_ptr<Trade> Trade::fromGraalNative(void *graalNative) noexcept {
    return TradeBase::fromGraalNative<Trade, dxfg_event_type_t, dxfg_trade_t, dxfg_event_clazz_t::DXFG_EVENT_TRADE>(
        graalNative);
}

} // namespace dxfcpp