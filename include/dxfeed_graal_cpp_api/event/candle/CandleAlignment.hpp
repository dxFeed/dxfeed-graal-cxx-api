// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "CandleSymbolAttribute.hpp"

#include "../market/MarketEventSymbols.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT CandleAlignment : public CandleSymbolAttribute<CandleAlignment> {
    static const CandleAlignment MIDNIGHT;
    static const CandleAlignment SESSION;
    static const CandleAlignment DEFAULT;
    static const std::string ATTRIBUTE_KEY;

  private:
    std::string string_;

    CandleAlignment(const std::string &string) : string_{string} {
    }

  public:
    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        return *this == DEFAULT ? MarketEventSymbols::removeAttributeStringByKey(symbol, ATTRIBUTE_KEY)
                                : MarketEventSymbols::changeAttributeStringByKey(symbol, ATTRIBUTE_KEY, toString());
    }

    void checkInAttributeImpl(const CandleSymbol &candleSymbol) const noexcept override {
    }

    std::string toString() const noexcept {
        return string_;
    }

    bool operator==(const CandleAlignment &candleAlignment) const {
        return string_ == candleAlignment.string_;
    }
};

} // namespace dxfcpp