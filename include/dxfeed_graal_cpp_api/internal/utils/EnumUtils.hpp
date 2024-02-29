// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace enum_utils {

/**
 * Returns a separated list of names of EventTypeEnum elements (CANDLE, QUOTE, TIME_AND_SALE etc).
 *
 * @param separator The list separator
 * @return A separated list of names
 */
DXFCPP_EXPORT std::string getEventTypeEnumNamesList(std::string separator = std::string(",")) noexcept;

/**
 * Returns a separated list of names of Market EventTypeEnum elements (QUOTE, TIME_AND_SALE etc).
 *
 * @param separator The list separator
 * @return A separated list of names
 */
DXFCPP_EXPORT std::string getMarketEventTypeEnumNamesList(std::string separator = std::string(",")) noexcept;

/**
 * Returns a separated list of class names of EventTypeEnum elements (Candle, Quote, TimeAndSale etc).
 *
 * @param separator The list separator
 * @return A separated list of class names
 */
DXFCPP_EXPORT std::string getEventTypeEnumClassNamesList(std::string separator = std::string(",")) noexcept;

/**
 * Returns a separated list of class names of Market EventTypeEnum elements (Quote, TimeAndSale etc).
 *
 * @param separator The list separator
 * @return A separated list of class names
 */
DXFCPP_EXPORT std::string getMarketEventTypeEnumClassNamesList(std::string separator = std::string(",")) noexcept;

} // namespace enum_utils

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
