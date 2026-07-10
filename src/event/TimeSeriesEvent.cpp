// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/TimeSeriesEvent.hpp"

DXFCPP_BEGIN_NAMESPACE

const IndexedEventSource &TimeSeriesEvent::getSource() const & noexcept {
    return IndexedEventSource::DEFAULT;
}

std::int64_t TimeSeriesEvent::getIndex() const noexcept {
    return 0;
}

std::int64_t TimeSeriesEvent::getEventId() const noexcept {
    return getIndex();
}

DXFCPP_END_NAMESPACE