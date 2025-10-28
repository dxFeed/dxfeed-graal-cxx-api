// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/event/candle/CandlePeriod.hpp>
#include <dxfeed_graal_cpp_api/event/candle/CandleType.hpp>

DXFCPP_BEGIN_NAMESPACE

const CandleType CandleType::TICK{"TICK", "t", 0LL};
const CandleType CandleType::SECOND{"SECOND", "s", 1000LL};
const CandleType CandleType::MINUTE{"MINUTE", "m", 60LL * 1000LL};
const CandleType CandleType::HOUR{"HOUR", "h", 60LL * 60LL * 1000L};
const CandleType CandleType::DAY{"DAY", "d", 24LL * 60LL * 60LL * 1000L};
const CandleType CandleType::WEEK{"WEEK", "w", 7LL * 24LL * 60LL * 60LL * 1000LL};
const CandleType CandleType::MONTH("MONTH", "mo", 30LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::OPTEXP("OPTEXP", "o", 30LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::YEAR("YEAR", "y", 365LL * 24LL * 60LL * 60LL * 1000LL);
const CandleType CandleType::VOLUME("VOLUME", "v", 0LL);
const CandleType CandleType::PRICE("PRICE", "p", 0LL);
const CandleType CandleType::PRICE_MOMENTUM("PRICE_MOMENTUM", "pm", 0LL);
const CandleType CandleType::PRICE_RENKO("PRICE_RENKO", "pr", 0LL);

const std::unordered_map<std::string, std::reference_wrapper<const CandleType>, StringHash, std::equal_to<>>
    CandleType::BY_STRING{
        {TICK.toString(), std::cref(TICK)},
        {SECOND.toString(), std::cref(SECOND)},
        {MINUTE.toString(), std::cref(MINUTE)},
        {HOUR.toString(), std::cref(HOUR)},
        {DAY.toString(), std::cref(DAY)},
        {WEEK.toString(), std::cref(WEEK)},
        {MONTH.toString(), std::cref(MONTH)},
        {OPTEXP.toString(), std::cref(OPTEXP)},
        {YEAR.toString(), std::cref(YEAR)},
        {VOLUME.toString(), std::cref(VOLUME)},
        {PRICE.toString(), std::cref(PRICE)},
        {PRICE_MOMENTUM.toString(), std::cref(PRICE_MOMENTUM)},
        {PRICE_RENKO.toString(), std::cref(PRICE_RENKO)},
    };

const std::vector<std::reference_wrapper<const CandleType>> CandleType::VALUES{
    std::cref(TICK),  std::cref(SECOND),         std::cref(MINUTE),      std::cref(HOUR), std::cref(DAY),
    std::cref(WEEK),  std::cref(MONTH),          std::cref(OPTEXP),      std::cref(YEAR), std::cref(VOLUME),
    std::cref(PRICE), std::cref(PRICE_MOMENTUM), std::cref(PRICE_RENKO),
};

const CandlePeriod CandlePeriod::TICK{1, CandleType::TICK};
const CandlePeriod CandlePeriod::DAY{1, CandleType::DAY};
const CandlePeriod CandlePeriod::DEFAULT = TICK;
const std::string CandlePeriod::ATTRIBUTE_KEY{};

DXFCPP_END_NAMESPACE