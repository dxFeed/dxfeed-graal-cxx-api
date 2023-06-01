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

const EventTypeEnum &Greeks::TYPE = EventTypeEnum::GREEKS;

std::shared_ptr<Greeks> Greeks::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_GREEKS) {
        return {};
    }

    try {
        auto graalGreeks = bit_cast<dxfg_greeks_t *>(graalNative);
        auto greeks = std::make_shared<Greeks>(dxfcpp::toString(graalGreeks->market_event.event_symbol));

        greeks->setEventTime(graalGreeks->market_event.event_time);

        greeks->data_ = {
            graalGreeks->event_flags, graalGreeks->index, graalGreeks->price,
            graalGreeks->volatility,  graalGreeks->delta, graalGreeks->gamma,
            graalGreeks->theta,       graalGreeks->rho,   graalGreeks->vega,
        };

        return greeks;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}
std::string Greeks::toString() const noexcept {
    return fmt::format(
        "Greeks{{{}, eventTime={}, eventFlags={:#x}, time={}, sequence={}, price={}, volatility={}, delta={}, "
        "gamma={}, theta={}, rho={}, vega={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlags().getMask(), formatTimeStampWithMillis(getTime()), getSequence(), getPrice(), getVolatility(),
        getDelta(), getGamma(), getTheta(), getRho(), getVega());
}

} // namespace dxfcpp