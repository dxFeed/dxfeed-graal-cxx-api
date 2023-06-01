// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &Trade::TYPE = EventTypeEnum::TRADE;

std::shared_ptr<Trade> Trade::fromGraalNative(void *graalNative) noexcept {
    return TradeBase::fromGraalNative<Trade, dxfg_event_type_t, dxfg_trade_t, dxfg_event_clazz_t::DXFG_EVENT_TRADE>(
        graalNative);
}

std::string Trade::toString() const noexcept { return fmt::format("Trade{{{}}}", baseFieldsToString()); }

} // namespace dxfcpp