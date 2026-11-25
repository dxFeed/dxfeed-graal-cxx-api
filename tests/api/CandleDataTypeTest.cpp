// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "CandleDataTypeJavaParity.hpp"

using namespace dxfcpp;

namespace {

const CandleDataType *findConstant(const std::string &name) {
    static const std::map<std::string, const CandleDataType *> CONSTANTS = {
        {"SOURCE", &CandleDataType::SOURCE}, {"THO", &CandleDataType::THO}, {"ALL", &CandleDataType::ALL},
        {"RTH", &CandleDataType::RTH},       {"ETH", &CandleDataType::ETH}, {"UNDEFINED", &CandleDataType::UNDEFINED},
    };
    const auto found = CONSTANTS.find(name);

    return found == CONSTANTS.end() ? nullptr : found->second;
}

// The constant equal to the data type, or "-" (as the Java probe prints a data type that is not a constant).
std::string constantName(const CandleDataType &dataType) {
    for (const auto *name : {"SOURCE", "THO", "ALL", "RTH", "ETH", "UNDEFINED"}) {
        if (dataType == *findConstant(name)) {
            return name;
        }
    }

    return "-";
}

} // namespace

TEST_CASE("The CandleDataType constants have the values of Java") {
    CHECK(CandleDataType::SOURCE.toString() == "source");
    CHECK(CandleDataType::THO.toString() == "tho");
    CHECK(CandleDataType::ALL.toString() == "all");
    CHECK(CandleDataType::RTH.toString() == "rth");
    CHECK(CandleDataType::ETH.toString() == "eth");
    CHECK(CandleDataType::UNDEFINED.toString().empty());
    CHECK(CandleDataType::DEFAULT == CandleDataType::UNDEFINED);
    CHECK(CandleDataType{} == CandleDataType::UNDEFINED);
    CHECK(std::string(CandleDataType::ATTRIBUTE_KEY) == "dt");
    CHECK(CandleDataType::THO.getValue() == CandleDataType::THO.toString());
}

TEST_CASE("CandleDataType::valueOf should match Java") {
    for (const auto &c : candle_data_type_java_parity::VALUE_OF) {
        const std::string input = c.input;

        CAPTURE(input);

        const auto dataType = CandleDataType::valueOf(input);

        CHECK(dataType.toString() == input);
        CHECK(dataType.getValue() == input);
        CHECK(constantName(dataType) == c.constant);
        CHECK((dataType == CandleDataType::DEFAULT) == c.isDefault);
        CHECK(CandleDataType::parse(input) == dataType);
    }
}

TEST_CASE("CandleDataType equality and hash compare the values") {
    CHECK(CandleDataType::THO == CandleDataType::valueOf("tho"));
    CHECK(CandleDataType::valueOf("xyz") == CandleDataType::valueOf("xyz"));
    CHECK_FALSE(CandleDataType::valueOf("xyz") == CandleDataType::valueOf("XYZ"));
    CHECK_FALSE(CandleDataType::THO == CandleDataType::valueOf("THO"));

    const std::unordered_set<CandleDataType> set{CandleDataType::THO, CandleDataType::valueOf("tho"),
                                                 CandleDataType::valueOf("xyz"), CandleDataType::valueOf("xyz")};

    CHECK(set.size() == 2);
}

TEST_CASE("A copy of a CandleDataType keeps its value after the original is destroyed") {
    std::optional<CandleDataType> original = CandleDataType::valueOf(std::string("custom") + "-value");
    const CandleDataType copy = *original;
    CandleDataType assigned = CandleDataType::THO;

    assigned = *original;
    original.reset();

    CHECK(copy.toString() == "custom-value");
    CHECK(assigned.toString() == "custom-value");
}

namespace {

// Where C++ deliberately differs from Java: Java removes the empty data type even when the rest of the symbol then
// parses differently (`A{=t}` gives `A`), C++ keeps it, so that normalization is idempotent; C++ removes the default
// session (see CandleSymbolTest).
const std::map<std::string, std::string> CXX_STR_OVERRIDES = {
    {"A{=t}{dt=}", "A{=t}{dt=}"},
    {"A{=d,dt=source,tho=false}", "A{=d,dt=source}"},
};

const std::map<std::string, std::string> CXX_BASE_OVERRIDES = {
    {"A{=t}{dt=}", "A{=t}"},
};

} // namespace

TEST_CASE("CandleSymbol::valueOf should match Java data types and normalize idempotently") {
    for (const auto &c : candle_data_type_java_parity::SYMBOL_VALUE_OF) {
        const std::string input = c.input;

        CAPTURE(input);

        const auto symbol = CandleSymbol::valueOf(input);
        const auto str = CXX_STR_OVERRIDES.find(input);
        const auto base = CXX_BASE_OVERRIDES.find(input);

        CHECK(symbol.toString() == (str == CXX_STR_OVERRIDES.end() ? std::string(c.str) : str->second));
        CHECK(symbol.getBaseSymbol() == (base == CXX_BASE_OVERRIDES.end() ? std::string(c.base) : base->second));
        REQUIRE(symbol.getDataType());
        CHECK(symbol.getDataType()->toString() == std::string(c.dataType));
        CHECK(constantName(*symbol.getDataType()) == c.constant);
        CHECK(symbol.getPeriod()->toString() == std::string(c.period));
        CHECK(symbol.getSession()->toString() == std::string(c.session));
        CHECK(CandleSymbol::valueOf(symbol.toString()).toString() == symbol.toString());
    }
}

TEST_CASE("CandleSymbol::valueOf with a data type should match Java") {
    for (const auto &c : candle_data_type_java_parity::SYMBOL_WITH_DATA_TYPE) {
        const std::string input = c.input;
        const auto dataType = CandleDataType::valueOf(c.dataType);

        CAPTURE(input);
        CAPTURE(dataType.toString());

        const auto symbol = CandleSymbol::valueOf(input, dataType);

        CHECK(symbol.toString() == std::string(c.str));
        REQUIRE(symbol.getDataType());
        CHECK(symbol.getDataType()->toString() == std::string(c.resultDataType));
        CHECK(dataType.changeAttributeForSymbol(input) == std::string(c.changed));
    }

    CHECK(CandleSymbol::valueOf(
              "A", {CandlePeriod::valueOf(5, CandleType::MINUTE), CandleDataType::RTH, CandleSession::REGULAR})
              .toString() == candle_data_type_java_parity::SYMBOL_WITH_ATTRIBUTES);
}

TEST_CASE("A CandleSymbolAttributeVariant holds a CandleDataType") {
    const CandleSymbolAttributeVariant attribute = CandleDataType::ETH;

    CHECK(std::holds_alternative<CandleDataType>(attribute));
    CHECK(CandleSymbol::valueOf("A{=m}", attribute).getDataType() == CandleDataType::ETH);
}

TEST_CASE("A copy of a CandleSymbol should keep the data type") {
    const auto original = CandleSymbol::valueOf("A{=m,dt=custom}");
    const auto copy = original;
    auto assigned = CandleSymbol::valueOf("B");

    assigned = original;

    CHECK(copy.getDataType() == CandleDataType::valueOf("custom"));
    CHECK(assigned.getDataType() == CandleDataType::valueOf("custom"));
    CHECK(CandleSymbol::valueOf("B").getDataType() == CandleDataType::DEFAULT);
}
