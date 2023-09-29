// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "../internal/Common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dxfcpp {

/**
 * Represents basic profile information about market instrument.
 * Please see <a href="http://www.dxfeed.com/downloads/documentation/dxFeed_Instrument_Profile_Format.pdf">Instrument Profile Format documentation</a>
 * for complete description.
 */
class DXFCPP_EXPORT InstrumentProfile final {
    struct Data {
        std::string type{};
        std::string symbol{};
        std::string description{};
        std::string localSymbol{};
        std::string localDescription{};
        std::string country{};
        std::string opol{};
        std::string exchangeData{};
        std::string exchanges{};
        std::string currency{};
        std::string baseCurrency{};
        std::string cfi{};
        std::string isin{};
        std::string sedol{};
        std::string cusip{};
        std::int32_t icb{};
        std::int32_t sic{};
        double multiplier = math::NaN;
        std::string product{};
        std::string underlying{};
        double spc = math::NaN;
        std::string additionalUnderlyings{};
        std::string mmy{};
        int32_t expiration{};
        int32_t lastTrade{};
        double strike = math::NaN;
        std::string optionType{};
        std::string expirationStyle{};
        std::string settlementStyle{};
        std::string priceIncrements{};
        std::string tradingHours{};
        std::vector<std::string> customFields{};
    };


};

}