// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Series::TYPE = EventTypeEnum::SERIES;

void Series::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    auto graalSeries = static_cast<dxfg_series_t *>(graalNative);

    data_ = {
        .eventFlags = graalSeries->event_flags,
        .index = graalSeries->index,
        .timeSequence = graalSeries->time_sequence,
        .expiration = graalSeries->expiration,
        .volatility = graalSeries->volatility,
        .callVolume = graalSeries->call_volume,
        .putVolume = graalSeries->put_volume,
        .putCallRatio = graalSeries->put_call_ratio,
        .forwardPrice = graalSeries->forward_price,
        .dividend = graalSeries->dividend,
        .interest = graalSeries->interest,
    };
}

void Series::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillGraalData(graalNative);

    auto graalSeries = static_cast<dxfg_series_t *>(graalNative);

    graalSeries->market_event.event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_SERIES;
    graalSeries->event_flags = data_.eventFlags;
    graalSeries->index = data_.index;
    graalSeries->time_sequence = data_.timeSequence;
    graalSeries->expiration = data_.expiration;
    graalSeries->volatility = data_.volatility;
    graalSeries->call_volume = data_.callVolume;
    graalSeries->put_volume = data_.putVolume;
    graalSeries->put_call_ratio = data_.putCallRatio;
    graalSeries->forward_price = data_.forwardPrice;
    graalSeries->dividend = data_.dividend;
    graalSeries->interest = data_.interest;
}

std::shared_ptr<Series> Series::fromGraal(void *graalNative) {
    if (!graalNative) {
        return {};
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_SERIES) {
        return {};
    }

    try {
        auto series = std::make_shared<Series>();

        series->fillData(graalNative);

        return series;
    } catch (...) {
        // TODO: error handling [EN-8232]
        return {};
    }
}

std::string Series::toString() const noexcept {
    return fmt::format(
        "Series{{{}, eventTime={}, eventFlags={:#x}, index={:#x}, time={}, sequence={}, expiration={}, "
        "volatility={}, callVolume={}, putVolume={}, putCallRatio={}, forwardPrice={}, dividend={}, interest={}}}",
        MarketEvent::getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(MarketEvent::getEventTime()),
        getEventFlagsMask().getMask(), getIndex(), TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(),
        day_util::getYearMonthDayByDayId(getExpiration()), dxfcpp::toString(getVolatility()),
        dxfcpp::toString(getCallVolume()), dxfcpp::toString(getPutVolume()), dxfcpp::toString(getPutCallRatio()),
        dxfcpp::toString(getForwardPrice()), dxfcpp::toString(getDividend()), dxfcpp::toString(getInterest()));
}

void *Series::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSeries = new dxfg_series_t{};

    fillGraalData(static_cast<void *>(graalSeries));

    return static_cast<void *>(graalSeries);
}

void Series::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_SERIES) {
        return;
    }

    auto graalSeries = static_cast<dxfg_series_t *>(graalNative);

    MarketEvent::freeGraalData(graalNative);

    delete graalSeries;
}

DXFCPP_END_NAMESPACE