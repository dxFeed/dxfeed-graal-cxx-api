// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../EventType.hpp"
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "CandleSymbol.hpp"

namespace dxfcpp {

struct EventMapper;

//TODO: implement

class Candle final : public EventTypeWithSymbol<CandleSymbol>, public TimeSeriesEvent, public LastingEvent {
    friend struct EventMapper;

  public:
    const CandleSymbol &getEventSymbol() const override { {}; }
    void setEventSymbol(const CandleSymbol &eventSymbol) override {}
    EventFlagsMask getEventFlags() const override { return EventFlagsMask(); }
    void setEventFlags(const EventFlagsMask &eventFlags) override {}
    void setIndex(std::int64_t index) override {}
    std::int64_t getTime() const override { return 0; }
};

} // namespace dxfcpp