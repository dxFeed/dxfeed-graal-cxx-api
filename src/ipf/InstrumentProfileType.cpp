// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

const InstrumentProfileType InstrumentProfileType::CURRENCY{0, "CURRENCY"};
const InstrumentProfileType InstrumentProfileType::FOREX{1, "FOREX"};
const InstrumentProfileType InstrumentProfileType::BOND{2, "BOND"};
const InstrumentProfileType InstrumentProfileType::INDEX{3, "INDEX"};
const InstrumentProfileType InstrumentProfileType::STOCK{4, "STOCK"};
const InstrumentProfileType InstrumentProfileType::ETF{5, "ETF"};
const InstrumentProfileType InstrumentProfileType::MUTUAL_FUND{6, "MUTUAL_FUND"};
const InstrumentProfileType InstrumentProfileType::MONEY_MARKET_FUND{7, "MONEY_MARKET_FUND"};
const InstrumentProfileType InstrumentProfileType::PRODUCT{8, "PRODUCT"};
const InstrumentProfileType InstrumentProfileType::FUTURE{9, "FUTURE"};
const InstrumentProfileType InstrumentProfileType::OPTION{10, "OPTION"};
const InstrumentProfileType InstrumentProfileType::WARRANT{11, "WARRANT"};
const InstrumentProfileType InstrumentProfileType::CFD{12, "CFD"};
const InstrumentProfileType InstrumentProfileType::SPREAD{13, "SPREAD"};
const InstrumentProfileType InstrumentProfileType::OTHER{14, "OTHER"};
const InstrumentProfileType InstrumentProfileType::REMOVED{15, "REMOVED"};

template <>
const std::unordered_map<InstrumentProfileType::CodeType, std::reference_wrapper<const InstrumentProfileType>>
    InstrumentProfileType::ParentType::ALL =
        [](std::initializer_list<std::reference_wrapper<const InstrumentProfileType>> orders) {
            std::unordered_map<InstrumentProfileType::CodeType, std::reference_wrapper<const InstrumentProfileType>>
                result{};

            for (auto &&o : orders) {
                result.emplace(o.get().getCode(), o);
            }

            return result;
        }({InstrumentProfileType::CURRENCY, InstrumentProfileType::FOREX, InstrumentProfileType::BOND,
           InstrumentProfileType::INDEX, InstrumentProfileType::STOCK, InstrumentProfileType::ETF,
           InstrumentProfileType::MUTUAL_FUND, InstrumentProfileType::MONEY_MARKET_FUND, InstrumentProfileType::PRODUCT,
           InstrumentProfileType::FUTURE, InstrumentProfileType::OPTION, InstrumentProfileType::WARRANT,
           InstrumentProfileType::CFD, InstrumentProfileType::SPREAD, InstrumentProfileType::OTHER,
           InstrumentProfileType::REMOVED});

DXFCPP_END_NAMESPACE