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

const EventTypeEnum &Series::TYPE = EventTypeEnum::SERIES;

std::shared_ptr<Series> Series::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_SERIES) {
        return {};
    }

    try {
        auto graalSeries = bit_cast<dxfg_series_t *>(graalNative);
        auto series = std::make_shared<Series>(dxfcpp::toString(graalSeries->market_event.event_symbol));

        series->setEventTime(graalSeries->market_event.event_time);
        series->data_ = {
            graalSeries->event_flags, graalSeries->index,          graalSeries->time_sequence,
            graalSeries->expiration,  graalSeries->volatility,     graalSeries->call_volume,
            graalSeries->put_volume,  graalSeries->put_call_ratio, graalSeries->forward_price,
            graalSeries->dividend,    graalSeries->interest,
        };

        return series;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string Series::toString() const noexcept {
    return fmt::format(
        "Series{{{}, eventTime={}, eventFlags={:#x}, index={:#x}, time={}, sequence={}, expiration={}, "
        "volatility={}, callVolume={}, putVolume={}, putCallRatio={}, forwardPrice={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), formatTimeStampWithMillis(MarketEvent::getEventTime()),
        getEventFlags().getMask(), getIndex(), formatTimeStampWithMillis(getTime()), getSequence(),
        day_util::getYearMonthDayByDayId(getExpiration()), getVolatility(), getCallVolume(), getPutVolume(),
        getPutCallRatio(), getForwardPrice(), getDividend(), getInterest());
}

} // namespace dxfcpp