// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

TEST_CASE("OrderSource::valueOf the method should work correctly with predefined sources") {
    REQUIRE(OrderSource::valueOf("COMPOSITE_ASK") == OrderSource::COMPOSITE_ASK);
    REQUIRE(OrderSource::valueOf("NTV") == OrderSource::NTV);
    REQUIRE(OrderSource::valueOf("NTV") != OrderSource::ntv);
    REQUIRE(OrderSource::valueOf("NTV2") != OrderSource::NTV);
}

// The expected values below are the results of the Java API (QD 3.355) for the same calls.

TEST_CASE("OrderSource::valueOf(id) returns the predefined sources and the sources of alphanumeric names") {
    const std::vector<std::pair<std::int32_t, std::string>> expected{
        {0, "DEFAULT"},      {1, "COMPOSITE_BID"}, {2, "COMPOSITE_ASK"}, {3, "REGIONAL_BID"},
        {4, "REGIONAL_ASK"}, {5, "AGGREGATE_BID"}, {6, "AGGREGATE_ASK"}, {7, "COMPOSITE"},
        {8, "REGIONAL"},     {0x4E5456, "NTV"},    {0x41424344, "ABCD"}};

    for (const auto &[id, name] : expected) {
        CAPTURE(id);

        const auto &source = OrderSource::valueOf(id);

        CHECK(source.id() == id);
        CHECK(source.name() == name);
    }
}

TEST_CASE("OrderSource::valueOf(id) rejects the ids that are not names of 1-4 alphanumeric characters") {
    for (const std::int32_t id : {-1, 42, 0x3F, 0x40, 0x7FFF'FFFF, static_cast<std::int32_t>(0x8000'0000), 0x0100'0000,
                                  0x0020'2020, 0x007F'7F7F, 0x4100'0000}) {
        CAPTURE(id);
        CHECK_THROWS(OrderSource::valueOf(id));
    }
}

TEST_CASE("OrderSource::valueOf(name) returns the predefined sources and the sources of 1-4 alphanumeric characters") {
    const std::vector<std::pair<std::string, std::int32_t>> expected{
        {"A", 65}, {"ABCD", 0x41424344}, {"ntv", 7'238'774}, {"NTV", 0x4E5456}, {"12", 12'594},
        {"1", 49}, {"COMPOSITE_BID", 1}, {"DEFAULT", 0}};

    for (const auto &[name, id] : expected) {
        CAPTURE(name);

        const auto source = OrderSource::valueOf(name);

        CHECK(source.id() == id);
        CHECK(source.name() == name);
    }
}

TEST_CASE("OrderSource::valueOf(name) rejects other names") {
    // "\xC3\x84\x42" is U+00C4 U+0042 in UTF-8.
    for (const std::string name : {"", "ABCDE", "a b", "\xC3\x84\x42", "EMPTY", "A-B", "~"}) {
        CAPTURE(name);
        CHECK_THROWS(OrderSource::valueOf(name));
    }
}

TEST_CASE("OrderSource::isSpecialSourceId is true for the ids 1-9") {
    for (std::int32_t id = -1; id <= 11; id++) {
        CAPTURE(id);
        CHECK(OrderSource::isSpecialSourceId(id) == (id >= 1 && id <= 9));
    }
}

TEST_CASE("OrderBase::setSource should preserve the low index bits") {
    constexpr std::int64_t index = 123'456'789LL;
    auto order = Order("AAPL");

    order.setIndex(index);
    order.setSource(OrderSource::NTV);

    CHECK(order.getSource() == OrderSource::NTV);
    CHECK((order.getIndex() & 0xffff'ffffLL) == index);
}

// MDAPI-429: these functions throw on invalid input, as in Java (IllegalArgumentException); with noexcept the process
// was terminated.
static_assert(!noexcept(std::declval<const OrderBase &>().getSource()));
static_assert(!noexcept(std::declval<OrderBase &>().setExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<Order &>().withIndex(0)));
static_assert(!noexcept(std::declval<Order &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<Order &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<SpreadOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<SpreadOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<SpreadOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<AnalyticOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withIndex(0)));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withExchangeCode('A')));
static_assert(!noexcept(std::declval<OtcMarketsOrder &>().withExchangeCode(std::int16_t{})));
static_assert(!noexcept(CmdArgsUtils::parseEventSources("")));

TEST_CASE("OrderBase::getSource throws InvalidArgumentException for an invalid source id in the index") {
    auto order = Order("AAPL");

    order.setIndex(0x00FFLL << 48);

    CHECK_THROWS_AS(order.getSource(), InvalidArgumentException);
}

TEST_CASE("OrderBase::setExchangeCode throws InvalidArgumentException for a code that is not a 7-bit character") {
    auto order = Order("AAPL");

    CHECK_THROWS_AS(order.setExchangeCode(std::int16_t{0xE2}), InvalidArgumentException);
    CHECK_THROWS_AS(order.setExchangeCode(std::int16_t{0x0100}), InvalidArgumentException);
    CHECK_THROWS_AS(order.setExchangeCode(std::int16_t{0x0151}), InvalidArgumentException);
    CHECK_THROWS_AS(order.setExchangeCode(static_cast<char>(0xE2)), InvalidArgumentException);

    order.setExchangeCode(std::int16_t{'Q'});

    CHECK(order.getExchangeCode() == 'Q');
}

template <typename O> void checkOrderWithThrows() {
    auto order = O("AAPL");

    CHECK_THROWS_AS(order.withIndex(-1), InvalidArgumentException);
    CHECK_THROWS_AS(order.withExchangeCode(static_cast<char>(0xE2)), InvalidArgumentException);
    CHECK_THROWS_AS(order.withExchangeCode(std::int16_t{0xE2}), InvalidArgumentException);
    CHECK(order.withIndex(1).withExchangeCode('Q').getExchangeCode() == 'Q');
}

TEST_CASE("withIndex and withExchangeCode of the order events throw InvalidArgumentException for invalid values") {
    SUBCASE("Order") {
        checkOrderWithThrows<Order>();
    }

    SUBCASE("SpreadOrder") {
        checkOrderWithThrows<SpreadOrder>();
    }

    SUBCASE("AnalyticOrder") {
        checkOrderWithThrows<AnalyticOrder>();
    }

    SUBCASE("OtcMarketsOrder") {
        checkOrderWithThrows<OtcMarketsOrder>();
    }
}

TEST_CASE("CmdArgsUtils::parseEventSources throws InvalidArgumentException for an invalid source name") {
    CHECK_THROWS_AS(CmdArgsUtils::parseEventSources("A-B"), InvalidArgumentException);
    CHECK_THROWS_AS(CmdArgsUtils::parseEventSources("ABCDE"), InvalidArgumentException);
    CHECK(CmdArgsUtils::parseEventSources("NTV, ntv").size() == 2);
}

// The predefined sources of the Java API (QDS 3.356): name, id and the flags of the publishable event types.

namespace {

constexpr std::uint32_t PUB_ORDER = 0x0001U;
constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
constexpr std::uint32_t PUB_OTC_MARKETS_ORDER = 0x0004U;
constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0008U;
constexpr std::uint32_t FULL_ORDER_BOOK = 0x0010U;
constexpr std::uint32_t PUB_NUAM_ORDER = 0x0020U;
constexpr std::uint32_t PUB_ORDER_IMBALANCE = 0x0040U;

struct PredefinedSource {
    std::string name;
    std::int32_t id;
    std::uint32_t flags;
};

const std::vector<PredefinedSource> &javaPredefinedSources() {
    static const std::vector<PredefinedSource> SOURCES{
        {"COMPOSITE_BID", 0x1, 0},
        {"COMPOSITE_ASK", 0x2, 0},
        {"REGIONAL_BID", 0x3, 0},
        {"REGIONAL_ASK", 0x4, 0},
        {"AGGREGATE_BID", 0x5, 0},
        {"AGGREGATE_ASK", 0x6, 0},
        {"COMPOSITE", 0x7, 0},
        {"REGIONAL", 0x8, 0},
        {"AGGREGATE", 0x9, 0},
        {"DEFAULT", 0x0,
         PUB_ORDER | PUB_ANALYTIC_ORDER | PUB_OTC_MARKETS_ORDER | PUB_SPREAD_ORDER | FULL_ORDER_BOOK | PUB_NUAM_ORDER |
             PUB_ORDER_IMBALANCE},
        {"NTV", 0x4E5456, PUB_ORDER | FULL_ORDER_BOOK | PUB_ORDER_IMBALANCE},
        {"ntv", 0x6E7476, PUB_ORDER},
        {"NFX", 0x4E4658, PUB_ORDER},
        {"ESPD", 0x45535044, PUB_ORDER},
        {"XNFI", 0x584E4649, PUB_ORDER},
        {"ICE", 0x494345, PUB_ORDER},
        {"ISE", 0x495345, PUB_ORDER | PUB_SPREAD_ORDER},
        {"DEA", 0x444541, PUB_ORDER},
        {"DEX", 0x444558, PUB_ORDER},
        {"dex", 0x646578, PUB_ORDER},
        {"BYX", 0x425958, PUB_ORDER},
        {"BZX", 0x425A58, PUB_ORDER},
        {"bzx", 0x627A78, PUB_ORDER},
        {"BATE", 0x42415445, PUB_ORDER},
        {"CHIX", 0x43484958, PUB_ORDER},
        {"CEUX", 0x43455558, PUB_ORDER},
        {"BXTR", 0x42585452, PUB_ORDER},
        {"IST", 0x495354, PUB_ORDER},
        {"BI20", 0x42493230, PUB_ORDER},
        {"ABE", 0x414245, PUB_ORDER},
        {"FAIR", 0x46414952, PUB_ORDER},
        {"GLBX", 0x474C4258, PUB_ORDER | PUB_ANALYTIC_ORDER},
        {"glbx", 0x676C6278, PUB_ORDER},
        {"ERIS", 0x45524953, PUB_ORDER},
        {"XEUR", 0x58455552, PUB_ORDER},
        {"xeur", 0x78657572, PUB_ORDER},
        {"CFE", 0x434645, PUB_ORDER},
        {"C2OX", 0x43324F58, PUB_ORDER},
        {"SMFE", 0x534D4645, PUB_ORDER},
        {"smfe", 0x736D6665, PUB_ORDER},
        {"iex", 0x696578, PUB_ORDER},
        {"MEMX", 0x4D454D58, PUB_ORDER},
        {"memx", 0x6D656D78, PUB_ORDER},
        {"OCEA", 0x4F434541, PUB_ORDER},
        {"ocea", 0x6F636561, PUB_ORDER},
        {"pink", 0x70696E6B, PUB_ORDER | PUB_OTC_MARKETS_ORDER},
        {"ARCA", 0x41524341, PUB_ORDER | PUB_ORDER_IMBALANCE},
        {"arca", 0x61726361, PUB_ORDER},
        {"CEDX", 0x43454458, PUB_ORDER},
        {"cedx", 0x63656478, PUB_ORDER},
        {"IGC", 0x494743, PUB_ORDER},
        {"igc", 0x696763, PUB_ORDER},
        {"EDX", 0x454458, PUB_ORDER},
        {"edx", 0x656478, PUB_ORDER},
        {"NUAM", 0x4E55414D, PUB_ORDER | FULL_ORDER_BOOK | PUB_NUAM_ORDER | PUB_ORDER_IMBALANCE},
        {"nuam", 0x6E75616D, PUB_ORDER | PUB_NUAM_ORDER},
        {"MOON", 0x4D4F4F4E, PUB_ORDER},
        {"moon", 0x6D6F6F6E, PUB_ORDER},
        {"OTCM", 0x4F54434D, PUB_ORDER},
        {"otcm", 0x6F74636D, PUB_ORDER},
        {"BOSS", 0x424F5353, PUB_ORDER},
        {"boss", 0x626F7373, PUB_ORDER},
        {"NEO", 0x4E454F, PUB_ORDER},
        {"neo", 0x6E656F, PUB_ORDER},
    };

    return SOURCES;
}

std::vector<std::string> names(std::span<const std::reference_wrapper<const OrderSource>> sources) {
    std::vector<std::string> result{};

    for (const auto &source : sources) {
        result.push_back(source.get().name());
    }

    return result;
}

std::vector<std::string> javaNamesWithFlag(std::uint32_t flag) {
    std::vector<std::string> result{};

    for (const auto &source : javaPredefinedSources()) {
        if ((source.flags & flag) != 0) {
            result.push_back(source.name);
        }
    }

    return result;
}

} // namespace

TEST_CASE("OrderSource predefined sources are the sources of the Java API with the same ids and flags") {
    REQUIRE(OrderSource::PREDEFINED_SOURCES.size() == 2 * javaPredefinedSources().size());

    for (const auto &[name, id, flags] : javaPredefinedSources()) {
        CAPTURE(name);

        const auto byName = OrderSource::PREDEFINED_SOURCES.find(name);
        const auto byId = OrderSource::PREDEFINED_SOURCES.find(id);

        REQUIRE(byName != OrderSource::PREDEFINED_SOURCES.end());
        REQUIRE(byId != OrderSource::PREDEFINED_SOURCES.end());

        const auto &source = byName->second.get();

        CHECK(&byId->second.get() == &source);
        CHECK(&OrderSource::valueOf(name) == &source);
        CHECK(&OrderSource::valueOf(id) == &source);
        CHECK(source.id() == id);
        CHECK(source.name() == name);

        CHECK(source.isPublishable(EventTypeEnum::ORDER) == ((flags & PUB_ORDER) != 0));
        CHECK(source.isPublishable(EventTypeEnum::ANALYTIC_ORDER) == ((flags & PUB_ANALYTIC_ORDER) != 0));
        CHECK(source.isPublishable(EventTypeEnum::OTC_MARKETS_ORDER) == ((flags & PUB_OTC_MARKETS_ORDER) != 0));
        CHECK(source.isPublishable(EventTypeEnum::SPREAD_ORDER) == ((flags & PUB_SPREAD_ORDER) != 0));
        CHECK(source.isPublishable(EventTypeEnum::NUAM_ORDER) == ((flags & PUB_NUAM_ORDER) != 0));
        CHECK(source.isPublishable(EventTypeEnum::ORDER_IMBALANCE) == ((flags & PUB_ORDER_IMBALANCE) != 0));
        CHECK(source.isFullOrderBook() == ((flags & FULL_ORDER_BOOK) != 0));
    }
}

TEST_CASE("OrderSource::publishable and OrderSource::fullOrderBook list the sources in the order of the Java API") {
    CHECK(names(OrderSource::publishable(EventTypeEnum::ORDER)) == javaNamesWithFlag(PUB_ORDER));
    CHECK(names(OrderSource::publishable(EventTypeEnum::ANALYTIC_ORDER)) == javaNamesWithFlag(PUB_ANALYTIC_ORDER));
    CHECK(names(OrderSource::publishable(EventTypeEnum::OTC_MARKETS_ORDER)) ==
          javaNamesWithFlag(PUB_OTC_MARKETS_ORDER));
    CHECK(names(OrderSource::publishable(EventTypeEnum::SPREAD_ORDER)) == javaNamesWithFlag(PUB_SPREAD_ORDER));
    CHECK(names(OrderSource::publishable(EventTypeEnum::NUAM_ORDER)) == javaNamesWithFlag(PUB_NUAM_ORDER));
    CHECK(names(OrderSource::publishable(EventTypeEnum::ORDER_IMBALANCE)) == javaNamesWithFlag(PUB_ORDER_IMBALANCE));
    CHECK(names(OrderSource::fullOrderBook()) == std::vector<std::string>{"DEFAULT", "NTV", "NUAM"});

    CHECK(names(OrderSource::publishable(EventTypeEnum::SPREAD_ORDER)) == std::vector<std::string>{"DEFAULT", "ISE"});
    CHECK(names(OrderSource::publishable(EventTypeEnum::NUAM_ORDER)) ==
          std::vector<std::string>{"DEFAULT", "NUAM", "nuam"});
    CHECK(names(OrderSource::publishable(EventTypeEnum::ORDER_IMBALANCE)) ==
          std::vector<std::string>{"DEFAULT", "NTV", "ARCA", "NUAM"});
    CHECK(names(OrderSource::publishable(EventTypeEnum::ORDER)).size() == javaPredefinedSources().size() - 9);
}

TEST_CASE("OrderSource::isPublishable and OrderSource::publishable reject the event types that are not orders") {
    for (const auto &eventType : {std::cref(EventTypeEnum::QUOTE), std::cref(EventTypeEnum::TIME_AND_SALE),
                                  std::cref(EventTypeEnum::CANDLE), std::cref(EventTypeEnum::SERIES)}) {
        CAPTURE(eventType.get().getName());
        CHECK_THROWS_AS(OrderSource::DEFAULT.isPublishable(eventType), InvalidArgumentException);
        CHECK_THROWS_AS(OrderSource::publishable(eventType), InvalidArgumentException);
    }
}

TEST_CASE("OrderSource user sources are not publishable") {
    const auto source = OrderSource::valueOf("ABCD");

    CHECK_FALSE(source.isPublishable(EventTypeEnum::ORDER));
    CHECK_FALSE(source.isPublishable(EventTypeEnum::SPREAD_ORDER));
    CHECK_FALSE(source.isFullOrderBook());
}
