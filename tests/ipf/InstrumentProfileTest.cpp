// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

using namespace std::literals;
using namespace dxfcpp;

TEST_CASE("InstrumentProfile string fields round-trip through the SDK") {
    struct StringField {
        std::string name;
        std::string javaName;
        std::function<void(const InstrumentProfile &, const std::string &)> set;
        std::function<std::string(const InstrumentProfile &)> get;
    };

#define DXFCPP_STRING_FIELD(Name, JavaName)                                                                            \
    StringField {                                                                                                      \
        #Name, JavaName,                                                                                               \
            [](const InstrumentProfile &ip, const std::string &value) {                                                \
                ip.set##Name(value);                                                                                   \
            },                                                                                                         \
            [](const InstrumentProfile &ip) {                                                                          \
                return ip.get##Name();                                                                                 \
            }                                                                                                          \
    }

    const std::vector<StringField> fields{
        DXFCPP_STRING_FIELD(Type, "TYPE"),
        DXFCPP_STRING_FIELD(Symbol, "SYMBOL"),
        DXFCPP_STRING_FIELD(Description, "DESCRIPTION"),
        DXFCPP_STRING_FIELD(LocalSymbol, "LOCAL_SYMBOL"),
        DXFCPP_STRING_FIELD(LocalDescription, "LOCAL_DESCRIPTION"),
        DXFCPP_STRING_FIELD(Country, "COUNTRY"),
        DXFCPP_STRING_FIELD(OPOL, "OPOL"),
        DXFCPP_STRING_FIELD(ExchangeData, "EXCHANGE_DATA"),
        DXFCPP_STRING_FIELD(Exchanges, "EXCHANGES"),
        DXFCPP_STRING_FIELD(Currency, "CURRENCY"),
        DXFCPP_STRING_FIELD(BaseCurrency, "BASE_CURRENCY"),
        DXFCPP_STRING_FIELD(CFI, "CFI"),
        DXFCPP_STRING_FIELD(ISIN, "ISIN"),
        DXFCPP_STRING_FIELD(SEDOL, "SEDOL"),
        DXFCPP_STRING_FIELD(CUSIP, "CUSIP"),
        DXFCPP_STRING_FIELD(Product, "PRODUCT"),
        DXFCPP_STRING_FIELD(Underlying, "UNDERLYING"),
        DXFCPP_STRING_FIELD(MMY, "MMY"),
        DXFCPP_STRING_FIELD(OptionType, "OPTION_TYPE"),
        DXFCPP_STRING_FIELD(ExpirationStyle, "EXPIRATION_STYLE"),
        DXFCPP_STRING_FIELD(SettlementStyle, "SETTLEMENT_STYLE"),
        DXFCPP_STRING_FIELD(PriceIncrements, "PRICE_INCREMENTS"),
        DXFCPP_STRING_FIELD(TradingHours, "TRADING_HOURS"),
    };

#undef DXFCPP_STRING_FIELD

    const auto profile = InstrumentProfile::create();

    for (const auto &field : fields) {
        CAPTURE(field.name);
        CHECK(field.get(*profile).empty());
    }

    // All fields hold distinct values at once, so a getter or setter wired to another field's SDK function shows up.
    for (const auto &field : fields) {
        field.set(*profile, "value of "s + field.name);
    }

    for (const auto &field : fields) {
        CAPTURE(field.name);

        const auto value = "value of "s + field.name;

        CHECK(field.get(*profile) == value);
        CHECK(profile->getField(field.javaName) == value);
    }

    for (const auto &field : fields) {
        CAPTURE(field.name);

        field.set(*profile, "");
        CHECK(field.get(*profile).empty());
    }

    profile->setAdditionalUnderlyings("SE 50");
    CHECK(profile->getAdditionalUnderlyings() == "SE 50");
}

TEST_CASE("InstrumentProfile numeric fields keep the values the old SDK functions used as error markers") {
    const auto profile = InstrumentProfile::create();
    constexpr auto minusInf = -std::numeric_limits<double>::infinity();

    SUBCASE("Double fields hold -inf") {
        profile->setStrike(minusInf);
        profile->setMultiplier(minusInf);
        profile->setSPC(minusInf);

        CHECK(profile->getStrike() == minusInf);
        CHECK(profile->getMultiplier() == minusInf);
        CHECK(profile->getSPC() == minusInf);
    }

    SUBCASE("Integer fields hold -1") {
        profile->setExpiration(-1);
        profile->setLastTrade(-1);
        profile->setICB(-1);
        profile->setSIC(-1);
        profile->setDateField("TEST_DATE", -1);

        CHECK(profile->getExpiration() == -1);
        CHECK(profile->getLastTrade() == -1);
        CHECK(profile->getICB() == -1);
        CHECK(profile->getSIC() == -1);
        CHECK(profile->getDateField("TEST_DATE") == -1);
    }

    SUBCASE("Ordinary values") {
        profile->setStrike(105.5);
        profile->setMultiplier(100.0);
        profile->setSPC(1.0);
        profile->setExpiration(19723);
        profile->setLastTrade(19722);
        profile->setICB(9535);
        profile->setSIC(7371);
        profile->setNumericField("TEST_NUMBER", 12.25);

        CHECK(profile->getStrike() == 105.5);
        CHECK(profile->getMultiplier() == 100.0);
        CHECK(profile->getSPC() == 1.0);
        CHECK(profile->getExpiration() == 19723);
        CHECK(profile->getLastTrade() == 19722);
        CHECK(profile->getICB() == 9535);
        CHECK(profile->getSIC() == 7371);
        CHECK(profile->getNumericField("TEST_NUMBER") == 12.25);
    }
}

TEST_CASE("InstrumentProfile::getNonEmptyCustomFieldNames() returns only the custom fields with a value") {
    const auto profile = InstrumentProfile::create();

    CHECK(profile->getNonEmptyCustomFieldNames().empty());

    profile->setField("TEST_A", "a");
    profile->setNumericField("TEST_B", 2.0);
    profile->setDateField("TEST_C", 19723);
    profile->setField("TEST_EMPTY", "");
    profile->setSymbol("AAPL");

    auto names = profile->getNonEmptyCustomFieldNames();

    std::sort(names.begin(), names.end());
    CHECK(names == std::vector<std::string>{"TEST_A", "TEST_B", "TEST_C"});
}

TEST_CASE("InstrumentProfile::create(ip) makes an independent copy") {
    const auto original = InstrumentProfile::create();

    original->setSymbol("AAPL");
    original->setStrike(10.0);
    original->setField("TEST_FIELD", "original");

    const auto copy = InstrumentProfile::create(original);

    CHECK(copy->getSymbol() == "AAPL");
    CHECK(copy->getStrike() == 10.0);
    CHECK(copy->getField("TEST_FIELD") == "original");

    copy->setSymbol("MSFT");
    copy->setStrike(20.0);
    copy->setField("TEST_FIELD", "copy");

    CHECK(original->getSymbol() == "AAPL");
    CHECK(original->getStrike() == 10.0);
    CHECK(original->getField("TEST_FIELD") == "original");
}

TEST_CASE("InstrumentProfile::setField() reports a Java error as JavaException") {
    const auto profile = InstrumentProfile::create();

    CHECK_THROWS_AS(profile->setField("EXPIRATION", "not a date"), JavaException);
    CHECK_THROWS_AS(profile->setField("STRIKE", "not a number"), JavaException);
}

TEST_CASE("InstrumentProfile::setDateField() should store a date") {
    constexpr std::int32_t dayId = 19723; // 2024-01-01
    const auto profile = InstrumentProfile::create();

    SUBCASE("Custom field") {
        profile->setDateField("TEST_DATE", dayId);

        CHECK(profile->getField("TEST_DATE") == "2024-01-01");
        CHECK(profile->getDateField("TEST_DATE") == dayId);
    }

    SUBCASE("Standard date field") {
        profile->setDateField("EXPIRATION", dayId);

        CHECK(profile->getField("EXPIRATION") == "2024-01-01");
        CHECK(profile->getDateField("EXPIRATION") == dayId);
        CHECK(profile->getExpiration() == dayId);
    }
}

TEST_CASE("InstrumentProfile::setField() and getField() round-trip a string through the SDK") {
    const auto profile = InstrumentProfile::create();

    SUBCASE("Custom field") {
        profile->setField("TEST_FIELD", "test value");

        CHECK(profile->getField("TEST_FIELD") == "test value");
    }

    SUBCASE("Standard field, std::string and non-ASCII UTF-8") {
        // "Privet" in Cyrillic, written as UTF-8 bytes.
        const std::string utf8 = "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";

        profile->setField("DESCRIPTION", utf8);

        CHECK(profile->getField("DESCRIPTION") == utf8);
    }
}
