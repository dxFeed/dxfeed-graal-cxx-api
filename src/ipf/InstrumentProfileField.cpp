// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfileField.hpp"

#include <array>
#include <functional>
#include <string_view>

DXFCPP_BEGIN_NAMESPACE

constinit const InstrumentProfileField InstrumentProfileField::TYPE{InstrumentProfileFieldEnum::TYPE, "TYPE"};
constinit const InstrumentProfileField InstrumentProfileField::SYMBOL{InstrumentProfileFieldEnum::SYMBOL, "SYMBOL"};
constinit const InstrumentProfileField InstrumentProfileField::DESCRIPTION{InstrumentProfileFieldEnum::DESCRIPTION,
                                                                           "DESCRIPTION"};
constinit const InstrumentProfileField InstrumentProfileField::LOCAL_SYMBOL{InstrumentProfileFieldEnum::LOCAL_SYMBOL,
                                                                            "LOCAL_SYMBOL"};
constinit const InstrumentProfileField InstrumentProfileField::LOCAL_DESCRIPTION{
    InstrumentProfileFieldEnum::LOCAL_DESCRIPTION, "LOCAL_DESCRIPTION"};
constinit const InstrumentProfileField InstrumentProfileField::COUNTRY{InstrumentProfileFieldEnum::COUNTRY, "COUNTRY"};
constinit const InstrumentProfileField InstrumentProfileField::OPOL{InstrumentProfileFieldEnum::OPOL, "OPOL"};
constinit const InstrumentProfileField InstrumentProfileField::EXCHANGE_DATA{InstrumentProfileFieldEnum::EXCHANGE_DATA,
                                                                             "EXCHANGE_DATA"};
constinit const InstrumentProfileField InstrumentProfileField::EXCHANGES{InstrumentProfileFieldEnum::EXCHANGES,
                                                                         "EXCHANGES"};
constinit const InstrumentProfileField InstrumentProfileField::CURRENCY{InstrumentProfileFieldEnum::CURRENCY,
                                                                        "CURRENCY"};
constinit const InstrumentProfileField InstrumentProfileField::BASE_CURRENCY{InstrumentProfileFieldEnum::BASE_CURRENCY,
                                                                             "BASE_CURRENCY"};
constinit const InstrumentProfileField InstrumentProfileField::CFI{InstrumentProfileFieldEnum::CFI, "CFI"};
constinit const InstrumentProfileField InstrumentProfileField::ISIN{InstrumentProfileFieldEnum::ISIN, "ISIN"};
constinit const InstrumentProfileField InstrumentProfileField::SEDOL{InstrumentProfileFieldEnum::SEDOL, "SEDOL"};
constinit const InstrumentProfileField InstrumentProfileField::CUSIP{InstrumentProfileFieldEnum::CUSIP, "CUSIP"};
constinit const InstrumentProfileField InstrumentProfileField::ICB{InstrumentProfileFieldEnum::ICB, "ICB",
                                                                   InstrumentProfileFieldTypeEnum::DOUBLE}; // double
constinit const InstrumentProfileField InstrumentProfileField::SIC{InstrumentProfileFieldEnum::SIC, "SIC",
                                                                   InstrumentProfileFieldTypeEnum::DOUBLE}; // double
constinit const InstrumentProfileField InstrumentProfileField::MULTIPLIER{
    InstrumentProfileFieldEnum::MULTIPLIER, "MULTIPLIER", InstrumentProfileFieldTypeEnum::DOUBLE}; // double
constinit const InstrumentProfileField InstrumentProfileField::PRODUCT{InstrumentProfileFieldEnum::PRODUCT, "PRODUCT"};
constinit const InstrumentProfileField InstrumentProfileField::UNDERLYING{InstrumentProfileFieldEnum::UNDERLYING,
                                                                          "UNDERLYING"};
constinit const InstrumentProfileField InstrumentProfileField::SPC{InstrumentProfileFieldEnum::SPC, "SPC",
                                                                   InstrumentProfileFieldTypeEnum::DOUBLE}; // double
constinit const InstrumentProfileField InstrumentProfileField::ADDITIONAL_UNDERLYINGS{
    InstrumentProfileFieldEnum::ADDITIONAL_UNDERLYINGS, "ADDITIONAL_UNDERLYINGS"};
constinit const InstrumentProfileField InstrumentProfileField::MMY{InstrumentProfileFieldEnum::MMY, "MMY"};
constinit const InstrumentProfileField InstrumentProfileField::EXPIRATION{
    InstrumentProfileFieldEnum::EXPIRATION, "EXPIRATION", InstrumentProfileFieldTypeEnum::DATE}; // date
constinit const InstrumentProfileField InstrumentProfileField::LAST_TRADE{
    InstrumentProfileFieldEnum::LAST_TRADE, "LAST_TRADE", InstrumentProfileFieldTypeEnum::DATE}; // date
constinit const InstrumentProfileField InstrumentProfileField::STRIKE{InstrumentProfileFieldEnum::STRIKE, "STRIKE",
                                                                      InstrumentProfileFieldTypeEnum::DOUBLE}; // double
constinit const InstrumentProfileField InstrumentProfileField::OPTION_TYPE{InstrumentProfileFieldEnum::OPTION_TYPE,
                                                                           "OPTION_TYPE"};
constinit const InstrumentProfileField InstrumentProfileField::EXPIRATION_STYLE{
    InstrumentProfileFieldEnum::EXPIRATION_STYLE, "EXPIRATION_STYLE"};
constinit const InstrumentProfileField InstrumentProfileField::SETTLEMENT_STYLE{
    InstrumentProfileFieldEnum::SETTLEMENT_STYLE, "SETTLEMENT_STYLE"};
constinit const InstrumentProfileField InstrumentProfileField::PRICE_INCREMENTS{
    InstrumentProfileFieldEnum::PRICE_INCREMENTS, "PRICE_INCREMENTS"};
constinit const InstrumentProfileField InstrumentProfileField::TRADING_HOURS{InstrumentProfileFieldEnum::TRADING_HOURS,
                                                                             "TRADING_HOURS"};

std::string InstrumentProfileField::getName() const {
    return std::string{name_};
}

namespace {
constinit const std::array FIELDS{
    std::cref(InstrumentProfileField::TYPE),
    std::cref(InstrumentProfileField::SYMBOL),
    std::cref(InstrumentProfileField::DESCRIPTION),
    std::cref(InstrumentProfileField::LOCAL_SYMBOL),
    std::cref(InstrumentProfileField::LOCAL_DESCRIPTION),
    std::cref(InstrumentProfileField::COUNTRY),
    std::cref(InstrumentProfileField::OPOL),
    std::cref(InstrumentProfileField::EXCHANGE_DATA),
    std::cref(InstrumentProfileField::EXCHANGES),
    std::cref(InstrumentProfileField::CURRENCY),
    std::cref(InstrumentProfileField::BASE_CURRENCY),
    std::cref(InstrumentProfileField::CFI),
    std::cref(InstrumentProfileField::ISIN),
    std::cref(InstrumentProfileField::SEDOL),
    std::cref(InstrumentProfileField::CUSIP),
    std::cref(InstrumentProfileField::ICB),
    std::cref(InstrumentProfileField::SIC),
    std::cref(InstrumentProfileField::MULTIPLIER),
    std::cref(InstrumentProfileField::PRODUCT),
    std::cref(InstrumentProfileField::UNDERLYING),
    std::cref(InstrumentProfileField::SPC),
    std::cref(InstrumentProfileField::ADDITIONAL_UNDERLYINGS),
    std::cref(InstrumentProfileField::MMY),
    std::cref(InstrumentProfileField::EXPIRATION),
    std::cref(InstrumentProfileField::LAST_TRADE),
    std::cref(InstrumentProfileField::STRIKE),
    std::cref(InstrumentProfileField::OPTION_TYPE),
    std::cref(InstrumentProfileField::EXPIRATION_STYLE),
    std::cref(InstrumentProfileField::SETTLEMENT_STYLE),
    std::cref(InstrumentProfileField::PRICE_INCREMENTS),
    std::cref(InstrumentProfileField::TRADING_HOURS),
};
} // namespace

std::optional<std::reference_wrapper<const InstrumentProfileField>>
InstrumentProfileField::find(const StringLike &name) noexcept {
    for (const auto &field : FIELDS) {
        if (field.get().name_ == std::string_view{name}) {
            return field;
        }
    }

    return std::nullopt;
}

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