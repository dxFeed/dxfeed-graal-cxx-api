// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const InstrumentProfileField InstrumentProfileField::TYPE{InstrumentProfileFieldEnum::TYPE, "TYPE"};
const InstrumentProfileField InstrumentProfileField::SYMBOL{InstrumentProfileFieldEnum::SYMBOL, "SYMBOL"};
const InstrumentProfileField InstrumentProfileField::DESCRIPTION{InstrumentProfileFieldEnum::DESCRIPTION,
                                                                 "DESCRIPTION"};
const InstrumentProfileField InstrumentProfileField::LOCAL_SYMBOL{InstrumentProfileFieldEnum::LOCAL_SYMBOL,
                                                                  "LOCAL_SYMBOL"};
const InstrumentProfileField InstrumentProfileField::LOCAL_DESCRIPTION{InstrumentProfileFieldEnum::LOCAL_DESCRIPTION,
                                                                       "LOCAL_DESCRIPTION"};
const InstrumentProfileField InstrumentProfileField::COUNTRY{InstrumentProfileFieldEnum::COUNTRY, "COUNTRY"};
const InstrumentProfileField InstrumentProfileField::OPOL{InstrumentProfileFieldEnum::OPOL, "OPOL"};
const InstrumentProfileField InstrumentProfileField::EXCHANGE_DATA{InstrumentProfileFieldEnum::EXCHANGE_DATA,
                                                                   "EXCHANGE_DATA"};
const InstrumentProfileField InstrumentProfileField::EXCHANGES{InstrumentProfileFieldEnum::EXCHANGES, "EXCHANGES"};
const InstrumentProfileField InstrumentProfileField::CURRENCY{InstrumentProfileFieldEnum::CURRENCY, "CURRENCY"};
const InstrumentProfileField InstrumentProfileField::BASE_CURRENCY{InstrumentProfileFieldEnum::BASE_CURRENCY,
                                                                   "BASE_CURRENCY"};
const InstrumentProfileField InstrumentProfileField::CFI{InstrumentProfileFieldEnum::CFI, "CFI"};
const InstrumentProfileField InstrumentProfileField::ISIN{InstrumentProfileFieldEnum::ISIN, "ISIN"};
const InstrumentProfileField InstrumentProfileField::SEDOL{InstrumentProfileFieldEnum::SEDOL, "SEDOL"};
const InstrumentProfileField InstrumentProfileField::CUSIP{InstrumentProfileFieldEnum::CUSIP, "CUSIP"};
const InstrumentProfileField InstrumentProfileField::ICB{InstrumentProfileFieldEnum::ICB, "ICB",
                                                         InstrumentProfileFieldTypeEnum::DOUBLE}; // double
const InstrumentProfileField InstrumentProfileField::SIC{InstrumentProfileFieldEnum::SIC, "SIC",
                                                         InstrumentProfileFieldTypeEnum::DOUBLE}; // double
const InstrumentProfileField InstrumentProfileField::MULTIPLIER{InstrumentProfileFieldEnum::MULTIPLIER, "MULTIPLIER",
                                                                InstrumentProfileFieldTypeEnum::DOUBLE}; // double
const InstrumentProfileField InstrumentProfileField::PRODUCT{InstrumentProfileFieldEnum::PRODUCT, "PRODUCT"};
const InstrumentProfileField InstrumentProfileField::UNDERLYING{InstrumentProfileFieldEnum::UNDERLYING, "UNDERLYING"};
const InstrumentProfileField InstrumentProfileField::SPC{InstrumentProfileFieldEnum::SPC, "SPC",
                                                         InstrumentProfileFieldTypeEnum::DOUBLE}; // double
const InstrumentProfileField InstrumentProfileField::ADDITIONAL_UNDERLYINGS{
    InstrumentProfileFieldEnum::ADDITIONAL_UNDERLYINGS, "ADDITIONAL_UNDERLYINGS"};
const InstrumentProfileField InstrumentProfileField::MMY{InstrumentProfileFieldEnum::MMY, "MMY"};
const InstrumentProfileField InstrumentProfileField::EXPIRATION{InstrumentProfileFieldEnum::EXPIRATION, "EXPIRATION",
                                                                InstrumentProfileFieldTypeEnum::DATE}; // date
const InstrumentProfileField InstrumentProfileField::LAST_TRADE{InstrumentProfileFieldEnum::LAST_TRADE, "LAST_TRADE",
                                                                InstrumentProfileFieldTypeEnum::DATE}; // date
const InstrumentProfileField InstrumentProfileField::STRIKE{InstrumentProfileFieldEnum::STRIKE, "STRIKE",
                                                            InstrumentProfileFieldTypeEnum::DOUBLE}; // double
const InstrumentProfileField InstrumentProfileField::OPTION_TYPE{InstrumentProfileFieldEnum::OPTION_TYPE,
                                                                 "OPTION_TYPE"};
const InstrumentProfileField InstrumentProfileField::EXPIRATION_STYLE{InstrumentProfileFieldEnum::EXPIRATION_STYLE,
                                                                      "EXPIRATION_STYLE"};
const InstrumentProfileField InstrumentProfileField::SETTLEMENT_STYLE{InstrumentProfileFieldEnum::SETTLEMENT_STYLE,
                                                                      "SETTLEMENT_STYLE"};
const InstrumentProfileField InstrumentProfileField::PRICE_INCREMENTS{InstrumentProfileFieldEnum::PRICE_INCREMENTS,
                                                                      "PRICE_INCREMENTS"};
const InstrumentProfileField InstrumentProfileField::TRADING_HOURS{InstrumentProfileFieldEnum::TRADING_HOURS,
                                                                   "TRADING_HOURS"};

const std::unordered_map<std::string, std::reference_wrapper<const InstrumentProfileField>> InstrumentProfileField::MAP{
    {InstrumentProfileField::TYPE.getName(), std::cref(InstrumentProfileField::TYPE)},
    {InstrumentProfileField::SYMBOL.getName(), std::cref(InstrumentProfileField::SYMBOL)},
    {InstrumentProfileField::DESCRIPTION.getName(), std::cref(InstrumentProfileField::DESCRIPTION)},
    {InstrumentProfileField::LOCAL_SYMBOL.getName(), std::cref(InstrumentProfileField::LOCAL_SYMBOL)},
    {InstrumentProfileField::LOCAL_DESCRIPTION.getName(), std::cref(InstrumentProfileField::LOCAL_DESCRIPTION)},
    {InstrumentProfileField::COUNTRY.getName(), std::cref(InstrumentProfileField::COUNTRY)},
    {InstrumentProfileField::OPOL.getName(), std::cref(InstrumentProfileField::OPOL)},
    {InstrumentProfileField::EXCHANGE_DATA.getName(), std::cref(InstrumentProfileField::EXCHANGE_DATA)},
    {InstrumentProfileField::EXCHANGES.getName(), std::cref(InstrumentProfileField::EXCHANGES)},
    {InstrumentProfileField::CURRENCY.getName(), std::cref(InstrumentProfileField::CURRENCY)},
    {InstrumentProfileField::BASE_CURRENCY.getName(), std::cref(InstrumentProfileField::BASE_CURRENCY)},
    {InstrumentProfileField::CFI.getName(), std::cref(InstrumentProfileField::CFI)},
    {InstrumentProfileField::ISIN.getName(), std::cref(InstrumentProfileField::ISIN)},
    {InstrumentProfileField::SEDOL.getName(), std::cref(InstrumentProfileField::SEDOL)},
    {InstrumentProfileField::CUSIP.getName(), std::cref(InstrumentProfileField::CUSIP)},
    {InstrumentProfileField::ICB.getName(), std::cref(InstrumentProfileField::ICB)},
    {InstrumentProfileField::SIC.getName(), std::cref(InstrumentProfileField::SIC)},
    {InstrumentProfileField::MULTIPLIER.getName(), std::cref(InstrumentProfileField::MULTIPLIER)},
    {InstrumentProfileField::PRODUCT.getName(), std::cref(InstrumentProfileField::PRODUCT)},
    {InstrumentProfileField::UNDERLYING.getName(), std::cref(InstrumentProfileField::UNDERLYING)},
    {InstrumentProfileField::SPC.getName(), std::cref(InstrumentProfileField::SPC)},
    {InstrumentProfileField::ADDITIONAL_UNDERLYINGS.getName(),
     std::cref(InstrumentProfileField::ADDITIONAL_UNDERLYINGS)},
    {InstrumentProfileField::MMY.getName(), std::cref(InstrumentProfileField::MMY)},
    {InstrumentProfileField::EXPIRATION.getName(), std::cref(InstrumentProfileField::EXPIRATION)},
    {InstrumentProfileField::LAST_TRADE.getName(), std::cref(InstrumentProfileField::LAST_TRADE)},
    {InstrumentProfileField::STRIKE.getName(), std::cref(InstrumentProfileField::STRIKE)},
    {InstrumentProfileField::OPTION_TYPE.getName(), std::cref(InstrumentProfileField::OPTION_TYPE)},
    {InstrumentProfileField::EXPIRATION_STYLE.getName(), std::cref(InstrumentProfileField::EXPIRATION_STYLE)},
    {InstrumentProfileField::SETTLEMENT_STYLE.getName(), std::cref(InstrumentProfileField::SETTLEMENT_STYLE)},
    {InstrumentProfileField::PRICE_INCREMENTS.getName(), std::cref(InstrumentProfileField::PRICE_INCREMENTS)},
    {InstrumentProfileField::TRADING_HOURS.getName(), std::cref(InstrumentProfileField::TRADING_HOURS)},
};

InstrumentProfileFieldEnum InstrumentProfileField::getFieldEnum() const {
    return fieldEnum_;
}

InstrumentProfileFieldTypeEnum InstrumentProfileField::getTypeEnum() const {
    return typeEnum_;
}

bool InstrumentProfileField::isNumericField() const {
    return numericField_;
}

} // namespace dxfcpp