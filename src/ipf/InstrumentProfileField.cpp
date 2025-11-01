// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfileField.hpp"

DXFCPP_BEGIN_NAMESPACE

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
    {TYPE.getName(), std::cref(TYPE)},
    {SYMBOL.getName(), std::cref(SYMBOL)},
    {DESCRIPTION.getName(), std::cref(DESCRIPTION)},
    {LOCAL_SYMBOL.getName(), std::cref(LOCAL_SYMBOL)},
    {LOCAL_DESCRIPTION.getName(), std::cref(LOCAL_DESCRIPTION)},
    {COUNTRY.getName(), std::cref(COUNTRY)},
    {OPOL.getName(), std::cref(OPOL)},
    {EXCHANGE_DATA.getName(), std::cref(EXCHANGE_DATA)},
    {EXCHANGES.getName(), std::cref(EXCHANGES)},
    {CURRENCY.getName(), std::cref(CURRENCY)},
    {BASE_CURRENCY.getName(), std::cref(BASE_CURRENCY)},
    {CFI.getName(), std::cref(CFI)},
    {ISIN.getName(), std::cref(ISIN)},
    {SEDOL.getName(), std::cref(SEDOL)},
    {CUSIP.getName(), std::cref(CUSIP)},
    {ICB.getName(), std::cref(ICB)},
    {SIC.getName(), std::cref(SIC)},
    {MULTIPLIER.getName(), std::cref(MULTIPLIER)},
    {PRODUCT.getName(), std::cref(PRODUCT)},
    {UNDERLYING.getName(), std::cref(UNDERLYING)},
    {SPC.getName(), std::cref(SPC)},
    {ADDITIONAL_UNDERLYINGS.getName(), std::cref(ADDITIONAL_UNDERLYINGS)},
    {MMY.getName(), std::cref(MMY)},
    {EXPIRATION.getName(), std::cref(EXPIRATION)},
    {LAST_TRADE.getName(), std::cref(LAST_TRADE)},
    {STRIKE.getName(), std::cref(STRIKE)},
    {OPTION_TYPE.getName(), std::cref(OPTION_TYPE)},
    {EXPIRATION_STYLE.getName(), std::cref(EXPIRATION_STYLE)},
    {SETTLEMENT_STYLE.getName(), std::cref(SETTLEMENT_STYLE)},
    {PRICE_INCREMENTS.getName(), std::cref(PRICE_INCREMENTS)},
    {TRADING_HOURS.getName(), std::cref(TRADING_HOURS)},
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

DXFCPP_END_NAMESPACE