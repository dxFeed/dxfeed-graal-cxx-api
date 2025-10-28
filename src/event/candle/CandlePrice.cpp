// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/candle/CandlePrice.hpp>

DXFCPP_BEGIN_NAMESPACE

const CandlePrice CandlePrice::LAST{"last"};
const CandlePrice CandlePrice::BID{"bid"};
const CandlePrice CandlePrice::ASK{"ask"};
const CandlePrice CandlePrice::MARK{"mark"};
const CandlePrice CandlePrice::SETTLEMENT{"s"};
const CandlePrice CandlePrice::DEFAULT = LAST;

const std::string CandlePrice::ATTRIBUTE_KEY{"price"};

const std::unordered_map<std::string, std::reference_wrapper<const CandlePrice>, StringHash, std::equal_to<>>
    CandlePrice::BY_STRING{
        {LAST.toString(), std::cref(LAST)},
        {BID.toString(), std::cref(BID)},
        {ASK.toString(), std::cref(ASK)},
        {MARK.toString(), std::cref(MARK)},
        {SETTLEMENT.toString(), std::cref(SETTLEMENT)},
    };

const std::vector<std::reference_wrapper<const CandlePrice>> CandlePrice::VALUES{
    std::cref(LAST), std::cref(BID), std::cref(ASK), std::cref(MARK), std::cref(SETTLEMENT),
};

DXFCPP_END_NAMESPACE