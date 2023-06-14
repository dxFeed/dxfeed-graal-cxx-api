// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"
#include "dxfeed_graal_cpp_api/api/osub/TimeSeriesSubscriptionSymbol.hpp"

namespace dxfcpp {

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(const SymbolWrapper &eventSymbol, int64_t fromTime)
    : eventSymbol_(std::make_unique<SymbolWrapper>(eventSymbol)), fromTime_(fromTime) {}

const std::unique_ptr<SymbolWrapper> &TimeSeriesSubscriptionSymbol::getEventSymbol() const { return eventSymbol_; }

int64_t TimeSeriesSubscriptionSymbol::getFromTime() const { return fromTime_; }

// TODO: Implement
void *TimeSeriesSubscriptionSymbol::toGraal() const noexcept { return nullptr; }

std::string TimeSeriesSubscriptionSymbol::toString() const noexcept {
    return eventSymbol_->toString() + "{fromTime=" + formatTimeStampWithMillis(fromTime_) + "}";
}

bool TimeSeriesSubscriptionSymbol::operator==(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
    return *eventSymbol_ == *timeSeriesSubscriptionSymbol.eventSymbol_;
}

bool TimeSeriesSubscriptionSymbol::operator<(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) const noexcept {
    return *eventSymbol_ < *timeSeriesSubscriptionSymbol.eventSymbol_;
}

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(
    const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept {
    eventSymbol_ = std::make_unique<SymbolWrapper>(*timeSeriesSubscriptionSymbol.eventSymbol_);
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;
}

TimeSeriesSubscriptionSymbol::TimeSeriesSubscriptionSymbol(
    TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept {
    eventSymbol_ = std::move(timeSeriesSubscriptionSymbol.eventSymbol_);
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;
}

TimeSeriesSubscriptionSymbol &
TimeSeriesSubscriptionSymbol::operator=(const TimeSeriesSubscriptionSymbol &timeSeriesSubscriptionSymbol) noexcept {
    eventSymbol_ = std::make_unique<SymbolWrapper>(*timeSeriesSubscriptionSymbol.eventSymbol_);
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;

    return *this;
}

TimeSeriesSubscriptionSymbol &
TimeSeriesSubscriptionSymbol::operator=(TimeSeriesSubscriptionSymbol &&timeSeriesSubscriptionSymbol) noexcept {
    eventSymbol_ = std::move(timeSeriesSubscriptionSymbol.eventSymbol_);
    fromTime_ = timeSeriesSubscriptionSymbol.fromTime_;

    return *this;
}

} // namespace dxfcpp