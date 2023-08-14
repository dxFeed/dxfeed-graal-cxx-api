// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

const CandlePrice CandlePrice::LAST{"last"};
const CandlePrice CandlePrice::BID{"bid"};
const CandlePrice CandlePrice::ASK{"ask"};
const CandlePrice CandlePrice::MARK{"mark"};
const CandlePrice CandlePrice::SETTLEMENT{"s"};
const CandlePrice CandlePrice::DEFAULT = LAST;

const std::string CandlePrice::ATTRIBUTE_KEY{"price"};

const std::unordered_map<std::string, std::reference_wrapper<const CandlePrice>> CandlePrice::BY_STRING{
    {CandlePrice::LAST.toString(), std::cref(CandlePrice::LAST)},
    {CandlePrice::BID.toString(), std::cref(CandlePrice::BID)},
    {CandlePrice::ASK.toString(), std::cref(CandlePrice::ASK)},
    {CandlePrice::MARK.toString(), std::cref(CandlePrice::MARK)},
    {CandlePrice::SETTLEMENT.toString(), std::cref(CandlePrice::SETTLEMENT)},
};

const std::vector<std::reference_wrapper<const CandlePrice>> CandlePrice::VALUES{
    std::cref(CandlePrice::LAST), std::cref(CandlePrice::BID),        std::cref(CandlePrice::ASK),
    std::cref(CandlePrice::MARK), std::cref(CandlePrice::SETTLEMENT),
};

} // namespace dxfcpp