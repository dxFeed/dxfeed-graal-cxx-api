// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventType.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"
#include "../LastingEvent.hpp"
#include "../TimeSeriesEvent.hpp"
#include "CandleSymbol.hpp"

namespace dxfcpp {

struct EventMapper;

// TODO: implement

class DXFCPP_EXPORT Candle final : public EventTypeWithSymbol<CandleSymbol>,
                                   public TimeSeriesEvent,
                                   public LastingEvent {
    friend struct EventMapper;

    CandleSymbol candleSymbol_{};

  public:
    const CandleSymbol &getEventSymbol() const override { return candleSymbol_; }
    void setEventSymbol(const CandleSymbol & /*eventSymbol*/) override {}
    EventFlagsMask getEventFlags() const override { return EventFlagsMask(); }
    void setEventFlags(const EventFlagsMask & /*eventFlags*/) override {}
    void setIndex(std::int64_t /*index*/) override {}
    std::int64_t getTime() const override { return 0; }
};

} // namespace dxfcpp