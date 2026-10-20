// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The library constants are read from the dynamic initializers of this translation unit. When the library is linked
// statically, these initializers run before the dynamic initializers of the library, so the constants and the
// functions that use them must not depend on dynamic initialization.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace dxfcpp;

namespace {

struct Check {
    std::string name;
    std::string actual;
    std::string expected;
};

std::vector<Check> &checks() {
    static std::vector<Check> result{};

    return result;
}

std::string run(const std::function<std::string()> &f) {
    try {
        return f();
    } catch (const std::exception &e) {
        return std::string("exception: ") + e.what();
    } catch (...) {
        return "unknown exception";
    }
}

std::string toString(bool value) {
    return value ? "true" : "false";
}

// The value read at static initialization and the value read in main() when all objects are initialized.
struct Capture {
    Capture(std::string name, std::function<std::string()> read, std::string expected = {}) {
        // Printed before reading, so a crash at static initialization shows the constant.
        std::printf("Reading %s\n", name.c_str());
        std::fflush(stdout);
        checks().push_back({std::move(name), run(read), std::move(expected)});
    }
};

// The constants are read again when the global variables are destroyed: the process fails if they are destroyed or
// changed. The observer is constructed before the variables below create the constants at the first call, so it is
// destroyed after the objects of the constants would be. On Windows the shared library destroys its objects when it is
// unloaded, after this observer, so only the static variant checks destruction there.
struct DestructionObserver {
    ~DestructionObserver() {
        const auto read = run([] {
            return std::string(CandleType::MINUTE.toString()) + " " + String::NUL() + " " +
                   CandleSymbol::NUL().toString() + " " + CandleType::BY_STRING().at("h").get().getName();
        });
        const bool ok = read == "m <null> <null> HOUR";

        std::printf("%s destruction: '%s'\n", ok ? "OK  " : "FAIL", read.c_str());
        std::fflush(stdout);

        if (!ok) {
            std::_Exit(1);
        }
    }
} destructionObserver{};

#define DXFC_CAPTURE(name, expression, expected)                                                                       \
    const Capture name {                                                                                               \
        #name,                                                                                                         \
            [] {                                                                                                       \
                return std::string(expression);                                                                        \
            },                                                                                                         \
            expected                                                                                                   \
    }

DXFC_CAPTURE(candleSymbol, CandleSymbol::valueOf("TEST", CandlePeriod::valueOf(1, CandleType::MINUTE)).toString(),
             "TEST{=m}");
DXFC_CAPTURE(candleTypeName, CandleType::MINUTE.toString(), "m");
DXFC_CAPTURE(candleTypeParse, toString(&CandleType::parse("h").get() == &CandleType::HOUR), "true");
DXFC_CAPTURE(candlePeriodParse, CandlePeriod::parse("5m").toString(), "5m");
DXFC_CAPTURE(candlePrice, CandlePrice::DEFAULT.toString(), "last");
DXFC_CAPTURE(eventFlag, std::to_string(EventFlag::SNAPSHOT_BEGIN.getFlag()), "4");
DXFC_CAPTURE(eventFlagName, EventFlag::SNAPSHOT_BEGIN.getName(), "SNAPSHOT_BEGIN");
DXFC_CAPTURE(indexedEventFlag, std::to_string(IndexedEvent::SNAPSHOT_END.getFlag()), "8");
DXFC_CAPTURE(eventType, EventTypeEnum::QUOTE.getName(), "QUOTE");
DXFC_CAPTURE(side, Side::BUY.getName(), "BUY");
DXFC_CAPTURE(sideValueOf, toString(&Side::valueOf(Side::BUY.getCode()) == &Side::BUY), "true");
DXFC_CAPTURE(defaultSource, IndexedEventSource::DEFAULT.name(), "DEFAULT");
DXFC_CAPTURE(orderSource, OrderSource::NTV.name(), "NTV");
DXFC_CAPTURE(orderSourceId, std::to_string(OrderSource::NTV.id()), {});
DXFC_CAPTURE(orderSourceByName, toString(&OrderSource::valueOf("NTV") == &OrderSource::NTV), "true");
DXFC_CAPTURE(orderSourceById, toString(&OrderSource::valueOf(OrderSource::NTV.id()) == &OrderSource::NTV), "true");
DXFC_CAPTURE(sessionFilter, toString(SessionFilter::TRADING.getTrading().value_or(false)), "true");
DXFC_CAPTURE(dayFilter, DayFilter::WEEK_DAY.getName() + ":" + std::to_string(DayFilter::WEEK_DAY.getDayOfWeekMask()),
             "WEEK_DAY:62");
DXFC_CAPTURE(wildcardSymbol, WildcardSymbol::ALL.getSymbol(), "*");
DXFC_CAPTURE(wildcardLiteral, toString("*"_ws == WildcardSymbol::ALL && "*"_wcs == WildcardSymbol::ALL), "true");
DXFC_CAPTURE(userOrderSource, OrderSource::valueOf("CNST").name(), "CNST");
DXFC_CAPTURE(userOrderSourceById, toString(&OrderSource::valueOf(0x434E5354) == &OrderSource::valueOf("CNST")), "true");
DXFC_CAPTURE(eventTypeLiteral, "QUOTE"_et.getName(), "QUOTE");
DXFC_CAPTURE(candlePriceParse, toString(&CandlePrice::parse("bid").get() == &CandlePrice::BID), "true");
DXFC_CAPTURE(candleAlignmentParse, toString(&CandleAlignment::parse("s").get() == &CandleAlignment::SESSION), "true");
DXFC_CAPTURE(candleSessionParse, toString(&CandleSession::parse("true").get() == &CandleSession::REGULAR), "true");
DXFC_CAPTURE(candleExchange, toString(CandleExchange::DEFAULT == CandleExchange::COMPOSITE), "true");
DXFC_CAPTURE(candlePriceLevel, toString(std::isnan(CandlePriceLevel::DEFAULT.getValue())), "true");
DXFC_CAPTURE(profileField, InstrumentProfileField::find("TRADING_HOURS").value().get().getName(), "TRADING_HOURS");
DXFC_CAPTURE(sessionType, SessionType::REGULAR.getName(), "REGULAR");
DXFC_CAPTURE(sideAll, std::to_string(Side::ALL.size()), "3");
DXFC_CAPTURE(candleTypes, std::to_string(CandleType::VALUES.size()), "13");

// The constants built at run time are functions: created at the first call.
DXFC_CAPTURE(attributeKey, CandlePrice::ATTRIBUTE_KEY() + CandleSession::ATTRIBUTE_KEY(), "pricetho");
DXFC_CAPTURE(candleTypeByString, CandleType::BY_STRING().at("h").get().getName(), "HOUR");
DXFC_CAPTURE(candleSessionByString, toString(&CandleSession::BY_STRING().at("true").get() == &CandleSession::REGULAR),
             "true");
DXFC_CAPTURE(eventTypeByName, EventTypeEnum::ALL_BY_NAME().at("TRADE").get().getClassName(), "Trade");
DXFC_CAPTURE(eventTypeById, EventTypeEnum::ALL_BY_ID().at(EventTypeEnum::QUOTE.getId()).get().getName(), "QUOTE");
DXFC_CAPTURE(predefinedSources, OrderSource::PREDEFINED_SOURCES().at("NTV").get().name(), "NTV");
DXFC_CAPTURE(candleSymbolNul, CandleSymbol::NUL().toString(), "<null>");
DXFC_CAPTURE(reservedPrefix, WildcardSymbol::RESERVED_PREFIX(), "*");
DXFC_CAPTURE(stringNul, String::NUL(), "<null>");
DXFC_CAPTURE(stringEmpty, toString(String::EMPTY().empty()), "true");

#undef DXFC_CAPTURE

} // namespace

int main() {
    // Re-read when all objects are initialized: the value expected for the checks that have no literal expectation.
    const std::vector<std::string> now{
        std::to_string(OrderSource::NTV.id()),
    };
    std::size_t nowIndex = 0;
    int failures = 0;

    for (auto &check : checks()) {
        if (check.expected.empty()) {
            check.expected = now.at(nowIndex++);
        }

        const bool ok = check.actual == check.expected;

        std::printf("%s %s: '%s'%s\n", ok ? "OK  " : "FAIL", check.name.c_str(), check.actual.c_str(),
                    ok ? "" : (" (expected '" + check.expected + "')").c_str());
        failures += ok ? 0 : 1;
    }

    // All the variables above were initialized.
    constexpr std::size_t expectedChecks = 42;

    if (checks().size() != expectedChecks) {
        std::printf("FAIL %zu checks instead of %zu\n", checks().size(), expectedChecks);
        failures += 1;
    }

    std::printf("%d of %zu checks failed\n", failures, checks().size());

    return failures == 0 ? 0 : 1;
}
