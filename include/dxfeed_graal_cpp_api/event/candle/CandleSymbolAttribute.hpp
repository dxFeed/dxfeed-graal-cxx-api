// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../detail/StringParam.hpp"

#include <string>

/**
 * \addtogroup dxfcpp_candle
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * Attribute of the CandleSymbol.
 */
struct DXFCPP_EXPORT CandleSymbolAttribute {
    /**
     * Returns candle event symbol string with this attribute set.
     *
     * @param symbol The original candle event symbol.
     * @return candle event symbol string with this attribute set.
     */
    virtual std::string changeAttributeForSymbol(StringParam symbol) const = 0;

    protected:
    // Not virtual: an attribute is not deleted through a pointer to this class. The destructor is trivial, so the
    // constants of the attributes are not destroyed at exit and can be used by the destructors of global variables.
    ~CandleSymbolAttribute() = default;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()