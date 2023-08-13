// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

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

const std::unordered_map<std::string, std::reference_wrapper<const CandleType>> CandleType::BY_STRING_{
    {CandleType::TICK.toString(), std::cref(CandleType::TICK)},
    {CandleType::SECOND.toString(), std::cref(CandleType::SECOND)},
    {CandleType::MINUTE.toString(), std::cref(CandleType::MINUTE)},
    {CandleType::HOUR.toString(), std::cref(CandleType::HOUR)},
    {CandleType::DAY.toString(), std::cref(CandleType::DAY)},
    {CandleType::WEEK.toString(), std::cref(CandleType::WEEK)},
    {CandleType::MONTH.toString(), std::cref(CandleType::MONTH)},
    {CandleType::OPTEXP.toString(), std::cref(CandleType::OPTEXP)},
    {CandleType::YEAR.toString(), std::cref(CandleType::YEAR)},
    {CandleType::VOLUME.toString(), std::cref(CandleType::VOLUME)},
    {CandleType::PRICE.toString(), std::cref(CandleType::PRICE)},
    {CandleType::PRICE_MOMENTUM.toString(), std::cref(CandleType::PRICE_MOMENTUM)},
    {CandleType::PRICE_RENKO.toString(), std::cref(CandleType::PRICE_RENKO)},
};

} // namespace dxfcpp