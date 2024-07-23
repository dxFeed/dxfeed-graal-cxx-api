// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(const SymbolWrapper &eventSymbol, int64_t fromTime)
    : IndexedEventSubscriptionSymbol(eventSymbol, IndexedEventSource::DEFAULT), fromTime_(fromTime) {
}

int64_t TimeSeriesSubscriptionSymbol::getFromTime() const {
    return fromTime_;
}

void *TimeSeriesSubscriptionSymbol::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::toGraal()");
    }

    auto *graalSymbol =
        new dxfg_time_series_subscription_symbol_t{.supper = {.type = dxfg_symbol_type_t::TIME_SERIES_SUBSCRIPTION},
                                                   .symbol = static_cast<dxfg_symbol_t *>(getEventSymbol()->toGraal()),
                                                   .from_time = fromTime_};

    return static_cast<void *>(graalSymbol);
}

void TimeSeriesSubscriptionSymbol::freeGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::freeGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        return;
    }

    auto *graalSymbol = static_cast<dxfg_time_series_subscription_symbol_t *>(graalNative);

    SymbolWrapper::freeGraal(graalSymbol->symbol);

    delete graalSymbol;
}

TimeSeriesSubscriptionSymbol TimeSeriesSubscriptionSymbol::fromGraal(void *graalNative) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("TimeSeriesSubscriptionSymbol::fromGraal(graal = " + toStringAny(graalNative) + ")");
    }

    if (graalNative == nullptr) {
        throw std::invalid_argument(
            "Unable to create TimeSeriesSubscriptionSymbol. The `graalNative` parameter is nullptr");
    }

    auto *graalSymbol = static_cast<dxfg_time_series_subscription_symbol_t *>(graalNative);

    return {SymbolWrapper::fromGraal(graalSymbol->symbol), graalSymbol->from_time};
}

std::string TimeSeriesSubscriptionSymbol::toString() const {
    if constexpr (Debugger::isDebug) {
        return "TimeSeriesSubscriptionSymbol{" + getEventSymbol()->toString() +
               ", fromTime = " + TimeFormat::DEFAULT_WITH_MILLIS.format(fromTime_) + "}";
    } else {
        return getEventSymbol()->toString() + "{fromTime=" + TimeFormat::DEFAULT_WITH_MILLIS.format(fromTime_) + "}";
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
    const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol)
    : IndexedEventSubscriptionSymbol(timeSeriesSubscriptionSymbol), fromTime_{timeSeriesSubscriptionSymbol.fromTime_} {
}

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(
    TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept
    : IndexedEventSubscriptionSymbol(std::move(timeSeriesSubscriptionSymbol)),
      fromTime_{timeSeriesSubscriptionSymbol.fromTime_} {
}

TimeSeriesSubscriptionSymbol &
TimeSeriesSubscriptionSymbol::operator=(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) {
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

DXFCPP_END_NAMESPACE