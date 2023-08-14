// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../symbols/SymbolWrapper.hpp"

#include "CandleAlignment.hpp"
#include "CandleExchange.hpp"
#include "CandlePeriod.hpp"
#include "CandlePrice.hpp"
#include "CandlePriceLevel.hpp"
#include "CandleSession.hpp"
#include "CandleSymbolAttribute.hpp"

#include <variant>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory>
#include <utility>

namespace dxfcpp {

using CandleSymbolAttributeVariant =
    std::variant<CandleExchange, CandlePrice, CandleSession, CandlePeriod, CandleAlignment, CandlePriceLevel>;

struct SymbolWrapper;

struct DXFCPP_EXPORT CandleSymbol {

  private:

    std::string symbol_{};
    std::string baseSymbol_{};
    //std::unordered_map<CandleSymbolAttributeType, CandleSymbolAttributeT> attributes_{};

  public:

    CandleSymbol() noexcept = default;

    virtual ~CandleSymbol() = default;

    virtual void *toGraal() const noexcept;

    static void freeGraal(void *graal) noexcept;

    static CandleSymbol fromGraal(void *graal) noexcept;
};

} // namespace dxfcpp