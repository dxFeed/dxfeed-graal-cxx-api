// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"

namespace dxfcpp {

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(const SymbolWrapper &eventSymbol, int64_t fromTime) noexcept
    : IndexedEventSubscriptionSymbol(eventSymbol, IndexedEventSource::DEFAULT), fromTime_(fromTime) {}

int64_t TimeSeriesSubscriptionSymbol::getFromTime() const { return fromTime_; }

void *TimeSeriesSubscriptionSymbol::toGraal() const noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::toGraal()");
    }

    auto *graalSymbol = new (std::nothrow) dxfg_time_series_subscription_symbol_t{
        {TIME_SERIES_SUBSCRIPTION}, dxfcpp::bit_cast<dxfg_symbol_t *>(getEventSymbol()->toGraal()), fromTime_};

    return dxfcpp::bit_cast<void *>(graalSymbol);
}

void TimeSeriesSubscriptionSymbol::freeGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::freeGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return;
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_time_series_subscription_symbol_t *>(graal);

    SymbolWrapper::freeGraal(graalSymbol->symbol);

    delete graalSymbol;
}

TimeSeriesSubscriptionSymbol TimeSeriesSubscriptionSymbol::fromGraal(void *graal) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::fromGraal(graal = " + toStringAny(graal) + ")");
    }

    if (graal == nullptr) {
        return {};
    }

    auto *graalSymbol = dxfcpp::bit_cast<dxfg_time_series_subscription_symbol_t *>(graal);

    return {SymbolWrapper::fromGraal(graalSymbol->symbol), graalSymbol->from_time};
}

std::string TimeSeriesSubscriptionSymbol::toString() const noexcept {
    if constexpr (Debugger::isDebug) {
        return "TimeSeriesSubscriptionSymbol{" + getEventSymbol()->toString() +
               ", fromTime = " + formatTimeStampWithMillis(fromTime_) + "}";
    } else {
        return getEventSymbol()->toString() + "{fromTime=" + formatTimeStampWithMillis(fromTime_) + "}";
    }
}

bool TimeSeriesSubscriptionSymbol::operator==(
    const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
    return *getEventSymbol() == *timeSeriesSubscriptionSymbol.getEventSymbol();
}

bool TimeSeriesSubscriptionSymbol::operator<(
    const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
    return *getEventSymbol() < *timeSeriesSubscriptionSymbol.getEventSymbol();
}

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(
    const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept
    : IndexedEventSubscriptionSymbol(timeSeriesSubscriptionSymbol), fromTime_{timeSeriesSubscriptionSymbol.fromTime_} {}

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(
    TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept
    : IndexedEventSubscriptionSymbol(std::move(timeSeriesSubscriptionSymbol)),
      fromTime_{timeSeriesSubscriptionSymbol.fromTime_} {}

TimeSeriesSubscriptionSymbol &
TimeSeriesSubscriptionSymbol::operator=(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept {
    if (this == &timeSeriesSubscriptionSymbol) {
        return *this;
    }

    IndexedEventSubscriptionSymbol::operator=(timeSeriesSubscriptionSymbol);
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;

    return *this;
}

TimeSeriesSubscriptionSymbol &
TimeSeriesSubscriptionSymbol::operator=(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept {
    if (this == &timeSeriesSubscriptionSymbol) {
        return *this;
    }

    IndexedEventSubscriptionSymbol::operator=(std::move(timeSeriesSubscriptionSymbol));
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;

    return *this;
}

} // namespace dxfcpp