// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace detail {

/**
 * Turns off the hints of CandleSymbol::valueOf(StringParam) about the likely mistakes in a candle symbol on this thread
 * while it lives. The library sets it where it creates a candle symbol from a string of the SDK (the symbol of an
 * event), which the user did not write.
 */
struct CandleSymbolHintsOff final {
    CandleSymbolHintsOff() noexcept;
    ~CandleSymbolHintsOff() noexcept;

    CandleSymbolHintsOff(const CandleSymbolHintsOff &) = delete;
    CandleSymbolHintsOff &operator=(const CandleSymbolHintsOff &) = delete;

  private:
    bool previous_;
};

} // namespace detail

DXFCPP_END_NAMESPACE
