// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/option/Series.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Series::TYPE = EventTypeEnum::SERIES;

void Series::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    MarketEvent::fillData(graalNative);

    const auto graalSeries = static_cast<dxfg_series_t *>(graalNative);

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

    graalSeries->market_event.event_type.clazz = DXFG_EVENT_SERIES;
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
        throw InvalidArgumentException("Unable to create Series. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SERIES) {
        throw InvalidArgumentException(fmt::format("Unable to create Series. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_SERIES)));
    }

    auto series = std::make_shared<Series>();

    series->fillData(graalNative);

    return series;
}

void Series::setSequence(std::int32_t sequence) {
    assert(sequence >= 0 && static_cast<std::uint32_t>(sequence) <= MAX_SEQUENCE);

    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid value for argument `sequence`: " + std::to_string(sequence));
    }

    data_.timeSequence = orOp(andOp(data_.timeSequence, ~MAX_SEQUENCE), sequence);
}

std::string Series::toString() const {
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
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalSeries = new dxfg_series_t{};

    fillGraalData(graalSeries);

    return graalSeries;
}

void Series::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_SERIES) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Series's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_SERIES)));
    }

    const auto graalSeries = static_cast<dxfg_series_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalSeries;
}

void Series::assign(std::shared_ptr<EventType> event) {
    MarketEvent::assign(event);

    if (const auto other = event->sharedAs<Series>(); other) {
        data_ = other->data_;
    }
}

DXFCPP_END_NAMESPACE