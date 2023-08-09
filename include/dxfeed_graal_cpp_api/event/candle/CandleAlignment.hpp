// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "CandleSymbolAttribute.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT CandleAlignment : public CandleSymbolAttribute<CandleAlignment> {
    static const CandleAlignment MIDNIGHT;
    static const CandleAlignment SESSION;
    static const CandleAlignment DEFAULT;
    static const std::string ATTRIBUTE_KEY;

  private:

    std::string string_;

    CandleAlignment(const std::string& string): string_{string} {}

  public:

    std::string changeAttributeForSymbol(const std::string &symbol) const noexcept override {
        return std::string();
    }

    void checkInAttributeImpl(const CandleSymbol &candleSymbol) const noexcept override {
    }


};

}