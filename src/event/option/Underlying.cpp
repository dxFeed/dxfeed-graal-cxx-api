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

const EventTypeEnum &Underlying::TYPE = EventTypeEnum::UNDERLYING;

std::shared_ptr<Underlying> Underlying::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_UNDERLYING) {
        return {};
    }

    try {
        auto graalUnderlying = bit_cast<dxfg_underlying_t *>(graalNative);
        auto underlying = std::make_shared<Underlying>(dxfcpp::toString(graalUnderlying->market_event.event_symbol));

        underlying->setEventTime(graalUnderlying->market_event.event_time);
        underlying->data_ = {
            graalUnderlying->event_flags,      graalUnderlying->index,           graalUnderlying->volatility,
            graalUnderlying->front_volatility, graalUnderlying->back_volatility, graalUnderlying->call_volume,
            graalUnderlying->put_volume,       graalUnderlying->put_call_ratio,
        };

        return underlying;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Underlying::toString() const noexcept {
    return fmt::format(
        "Underlying{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, volatility={}, frontVolatility={}, "
        "backVolatility={}, callVolume={}, putVolume={}, putCallRatio={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getSequence(), getVolatility(),
        getFrontVolatility(), getBackVolatility(), getCallVolume(), getPutVolume(), getPutCallRatio());
}

} // namespace dxfcpp