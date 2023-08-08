// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <string>

namespace dxfcpp {

class CandleSymbol;

/**
 * Attribute of the {@link CandleSymbol}.
 * @tparam A the attribute class.
 */
template <typename A>
struct DXFCPP_EXPORT CandleSymbolAttribute {
    /**
     * Returns candle event symbol string with this attribute set.
     *
     * @param symbol The original candle event symbol.
     * @return candle event symbol string with this attribute set.
     */
    virtual std::string changeAttributeForSymbol(const std::string& symbol) const noexcept = 0;

    /**
     * Internal method that initializes attribute in the candle symbol.
     *
     * @param candleSymbol The candle symbol.
     */
    virtual void checkInAttributeImpl(const CandleSymbol& candleSymbol) const noexcept = 0;
};

}