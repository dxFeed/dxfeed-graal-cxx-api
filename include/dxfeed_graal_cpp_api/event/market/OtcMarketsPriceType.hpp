// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../../internal/Enum.hpp"

#include <cstdint>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Type of prices on the OTC Markets.
 * Please see <a href="https://downloads.dxfeed.com/specifications/OTC_Markets_Data_Display_Requirements.pdf">OTC Markets Data Display Requirements</a>
 * section "4.2 Quote Price Types" for complete description.
 */
struct OtcMarketsPriceType : Enum<OtcMarketsPriceType, std::uint32_t> {
    using Enum::Enum;

    /**
     * Unpriced quotes are an indication of interest (IOI) in a security
     * used when a trader does not wish to show a price or size.
     * Unpriced, name-only quotes are also used as the other side of a one-sided, priced quote.
     * Unpriced quotes may not have a Quote Access Payment (QAP) value.
     */
    static const DXFCPP_EXPORT OtcMarketsPriceType UNPRICED;

    /**
     * Actual (Priced) is the actual amount a trader is willing to buy or sell securities.
     */
    static const DXFCPP_EXPORT OtcMarketsPriceType ACTUAL;

    /**
     * Offer Wanted/Bid Wanted (OW/BW) is used to solicit sellers/buyers,
     * without displaying actual price or size.
     * OW/BW quotes may not have a Quote Access Payment (QAP) value.
     */
    static const DXFCPP_EXPORT OtcMarketsPriceType WANTED;
};

template <>
const std::unordered_map<OtcMarketsPriceType::CodeType, std::reference_wrapper<const OtcMarketsPriceType>>
    OtcMarketsPriceType::ParentType::ALL;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()