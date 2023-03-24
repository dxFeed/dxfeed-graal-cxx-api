// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"
#include "../TimeSeriesEvent.hpp"
#include "../market/MarketEvent.hpp"
namespace dxfcpp {

struct EventMapper;

// TODO: implement

class TimeAndSale final : public MarketEvent, public TimeSeriesEvent {

};

} // namespace dxfcpp