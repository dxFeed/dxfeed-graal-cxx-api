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

const EventTypeEnum &TheoPrice::TYPE = EventTypeEnum::THEO_PRICE;

std::shared_ptr<TheoPrice> TheoPrice::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_THEO_PRICE) {
        return {};
    }

    try {
        auto graalTheoPrice = bit_cast<dxfg_theo_price_t *>(graalNative);
        auto theoPrice = std::make_shared<TheoPrice>(dxfcpp::toString(graalTheoPrice->market_event.event_symbol));

        theoPrice->setEventTime(graalTheoPrice->market_event.event_time);
        theoPrice->data_ = {
            graalTheoPrice->event_flags,      graalTheoPrice->index,    graalTheoPrice->price,
            graalTheoPrice->underlying_price, graalTheoPrice->delta,    graalTheoPrice->gamma,
            graalTheoPrice->dividend,         graalTheoPrice->interest,
        };

        return theoPrice;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string TheoPrice::toString() const noexcept {
    return fmt::format(
        "TheoPrice{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, underlyingPrice={}, "
        "delta={}, gamma={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getSequence(), getPrice(),
        getUnderlyingPrice(), getDelta(), getGamma(), getDividend(), getInterest());
}

} // namespace dxfcpp