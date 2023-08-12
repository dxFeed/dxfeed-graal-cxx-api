// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <string>

namespace dxfcpp {

enum class CandleSymbolAttributeType {
    CANDLE_EXCHANGE,
    CANDLE_PRICE,
    CANDLE_SESSION,
    CANDLE_PERIOD,
    CANDLE_ALIGNMENT,
    CANDLE_PRICE_LEVEL,
};

class CandleSymbol;

/**
 * Attribute of the {@link CandleSymbol}.
 */
struct DXFCPP_EXPORT CandleSymbolAttribute {
    /**
     * Returns candle event symbol string with this attribute set.
     *
     * @param symbol The original candle event symbol.
     * @return candle event symbol string with this attribute set.
     */
    virtual std::string changeAttributeForSymbol(const std::string& symbol) const noexcept = 0;
};

}