// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

const CandleAlignment CandleAlignment::MIDNIGHT{"m"};
const CandleAlignment CandleAlignment::SESSION{"s"};
const CandleAlignment CandleAlignment::DEFAULT = MIDNIGHT;
const std::string CandleAlignment::ATTRIBUTE_KEY{"a"};

const std::unordered_map<std::string, std::reference_wrapper<const CandleAlignment>, dxfcpp::StringHash,
                         std::equal_to<>>
    CandleAlignment::BY_STRING{
        {CandleAlignment::MIDNIGHT.toString(), std::cref(CandleAlignment::MIDNIGHT)},
        {CandleAlignment::SESSION.toString(), std::cref(CandleAlignment::SESSION)},
    };

const std::vector<std::reference_wrapper<const CandleAlignment>> CandleAlignment::VALUES{
    std::cref(CandleAlignment::MIDNIGHT),
    std::cref(CandleAlignment::SESSION),
};

DXFCPP_END_NAMESPACE