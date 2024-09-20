// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>

DXFCPP_BEGIN_NAMESPACE

struct StringLikeWrapper;

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
    virtual std::string changeAttributeForSymbol(const dxfcpp::StringLikeWrapper &symbol) const = 0;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()