// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"
#include "MarketEvent.hpp"
#include "TradeBase.hpp"

namespace dxfcpp {

struct EventMapper;

// TODO: implement

class Trade final : public TradeBase {
    friend struct EventMapper;

    static std::shared_ptr<Trade> fromGraalNative(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &Type;

    /// Creates new trade with default values.
    Trade() noexcept = default;

    /**
     * Creates new trade with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit Trade(std::string eventSymbol) noexcept : TradeBase(std::move(eventSymbol)) {}

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept { return fmt::format("Trade{{{}}}", baseFieldsToString()); }

    template <typename OStream> friend OStream &operator<<(OStream &os, const Trade &e) { return os << e.toString(); }

    template <typename OStream> friend OStream &operator<<(OStream &os, const std::shared_ptr<Trade> &e) {
        return os << e->toString();
    }
};

} // namespace dxfcpp