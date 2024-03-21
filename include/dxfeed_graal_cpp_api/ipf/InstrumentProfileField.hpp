// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

enum class InstrumentProfileFieldEnum {
    TYPE,
    SYMBOL,
    DESCRIPTION,
    LOCAL_SYMBOL,
    LOCAL_DESCRIPTION,
    COUNTRY,
    OPOL,
    EXCHANGE_DATA,
    EXCHANGES,
    CURRENCY,
    BASE_CURRENCY,
    CFI,
    ISIN,
    SEDOL,
    CUSIP,
    ICB,        // double
    SIC,        // double
    MULTIPLIER, // double
    PRODUCT,
    UNDERLYING,
    SPC, // double
    ADDITIONAL_UNDERLYINGS,
    MMY,
    EXPIRATION, // date
    LAST_TRADE, // date
    STRIKE,     // double
    OPTION_TYPE,
    EXPIRATION_STYLE,
    SETTLEMENT_STYLE,
    PRICE_INCREMENTS,
    TRADING_HOURS
};

enum class InstrumentProfileFieldTypeEnum { STRING, DOUBLE, DATE };

class DXFCPP_EXPORT InstrumentProfileField {
    InstrumentProfileFieldEnum fieldEnum_;
    std::string name_;
    InstrumentProfileFieldTypeEnum typeEnum_;
    bool numericField_;

    InstrumentProfileField(InstrumentProfileFieldEnum fieldEnum, std::string name,
                           InstrumentProfileFieldTypeEnum typeEnum) noexcept
        : fieldEnum_{fieldEnum}, name_{std::move(name)}, typeEnum_{typeEnum},
          numericField_{typeEnum != InstrumentProfileFieldTypeEnum::STRING} {
    }

    explicit InstrumentProfileField(InstrumentProfileFieldEnum fieldEnum, const std::string &name) noexcept
        : InstrumentProfileField(fieldEnum, name, InstrumentProfileFieldTypeEnum::STRING) {
    }

  public:
    const static InstrumentProfileField TYPE;
    const static InstrumentProfileField SYMBOL;
    const static InstrumentProfileField DESCRIPTION;
    const static InstrumentProfileField LOCAL_SYMBOL;
    const static InstrumentProfileField LOCAL_DESCRIPTION;
    const static InstrumentProfileField COUNTRY;
    const static InstrumentProfileField OPOL;
    const static InstrumentProfileField EXCHANGE_DATA;
    const static InstrumentProfileField EXCHANGES;
    const static InstrumentProfileField CURRENCY;
    const static InstrumentProfileField BASE_CURRENCY;
    const static InstrumentProfileField CFI;
    const static InstrumentProfileField ISIN;
    const static InstrumentProfileField SEDOL;
    const static InstrumentProfileField CUSIP;
    const static InstrumentProfileField ICB;        // double
    const static InstrumentProfileField SIC;        // double
    const static InstrumentProfileField MULTIPLIER; // double
    const static InstrumentProfileField PRODUCT;
    const static InstrumentProfileField UNDERLYING;
    const static InstrumentProfileField SPC; // double
    const static InstrumentProfileField ADDITIONAL_UNDERLYINGS;
    const static InstrumentProfileField MMY;
    const static InstrumentProfileField EXPIRATION; // date
    const static InstrumentProfileField LAST_TRADE; // date
    const static InstrumentProfileField STRIKE;     // double
    const static InstrumentProfileField OPTION_TYPE;
    const static InstrumentProfileField EXPIRATION_STYLE;
    const static InstrumentProfileField SETTLEMENT_STYLE;
    const static InstrumentProfileField PRICE_INCREMENTS;
    const static InstrumentProfileField TRADING_HOURS;

    [[nodiscard]] const std::string &getName() const & noexcept {
        return name_;
    }

  private:
    const static std::unordered_map<std::string, std::reference_wrapper<const InstrumentProfileField>> MAP;

  public:
    /**
     * Returns cref to field for specified name or std::nullopt if field is not found.
     */
    static std::optional<std::reference_wrapper<const InstrumentProfileField>> find(const std::string &name) noexcept {
        if (MAP.contains(name)) {
            return MAP.at(name);
        }

        return std::nullopt;
    }

    InstrumentProfileFieldEnum getFieldEnum() const;

    InstrumentProfileFieldTypeEnum getTypeEnum() const;

    bool isNumericField() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()