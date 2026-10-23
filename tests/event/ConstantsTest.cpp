// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_cpp_api/isolated/util/IsolatedTimePeriod.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

namespace {

template <typename T> struct Element {
    std::reference_wrapper<const T> constant;
    std::string name;
};

#define DXFC_ELEMENT(T, NAME)                                                                                          \
    Element<T> {                                                                                                       \
        std::cref(T::NAME), #NAME                                                                                      \
    }

// The collection of the constants holds the constants themselves, in the declaration order.
template <typename T>
void checkSameConstants(std::span<const std::reference_wrapper<const T>> values,
                        const std::vector<Element<T>> &expected) {
    REQUIRE(values.size() == expected.size());

    for (std::size_t i = 0; i < expected.size(); i++) {
        CHECK(&values[i].get() == &expected[i].constant.get());
    }
}

// An Enum: the names, the collection of the elements and the lookup by code.
template <typename T> void checkEnum(const std::vector<Element<T>> &expected) {
    checkSameConstants<T>(T::ALL, expected);

    std::set<typename T::CodeType> codes{};

    for (const auto &[constant, name] : expected) {
        const auto &element = constant.get();

        CHECK(element.getName() == name);
        CHECK(element.toString() == name);
        CHECK(&T::valueOf(element.getCode()) == &element);
        codes.insert(element.getCode());
    }

    CHECK(codes.size() == expected.size());
}

} // namespace

TEST_CASE("The elements of Enum are its constants and are found by code") {
    checkEnum<Side>({DXFC_ELEMENT(Side, UNDEFINED), DXFC_ELEMENT(Side, BUY), DXFC_ELEMENT(Side, SELL)});
    checkEnum<Scope>({DXFC_ELEMENT(Scope, COMPOSITE), DXFC_ELEMENT(Scope, REGIONAL), DXFC_ELEMENT(Scope, AGGREGATE),
                      DXFC_ELEMENT(Scope, ORDER)});
    checkEnum<Direction>({DXFC_ELEMENT(Direction, UNDEFINED), DXFC_ELEMENT(Direction, DOWN),
                          DXFC_ELEMENT(Direction, ZERO_DOWN), DXFC_ELEMENT(Direction, ZERO),
                          DXFC_ELEMENT(Direction, ZERO_UP), DXFC_ELEMENT(Direction, UP)});
    checkEnum<IcebergType>({DXFC_ELEMENT(IcebergType, UNDEFINED), DXFC_ELEMENT(IcebergType, NATIVE),
                            DXFC_ELEMENT(IcebergType, SYNTHETIC)});
    checkEnum<AuctionType>({DXFC_ELEMENT(AuctionType, UNDEFINED), DXFC_ELEMENT(AuctionType, OPENING),
                            DXFC_ELEMENT(AuctionType, CLOSING), DXFC_ELEMENT(AuctionType, REOPENING),
                            DXFC_ELEMENT(AuctionType, OTHER)});
    checkEnum<ImbalanceSide>({DXFC_ELEMENT(ImbalanceSide, UNDEFINED), DXFC_ELEMENT(ImbalanceSide, BUY),
                              DXFC_ELEMENT(ImbalanceSide, SELL), DXFC_ELEMENT(ImbalanceSide, NO_IMBALANCE),
                              DXFC_ELEMENT(ImbalanceSide, INSUFFICIENT_ORDERS), DXFC_ELEMENT(ImbalanceSide, PAUSED)});
    checkEnum<NuamOrderType>({DXFC_ELEMENT(NuamOrderType, UNDEFINED), DXFC_ELEMENT(NuamOrderType, LIMIT),
                              DXFC_ELEMENT(NuamOrderType, MARKET), DXFC_ELEMENT(NuamOrderType, MARKET_TO_LIMIT),
                              DXFC_ELEMENT(NuamOrderType, BEST_ORDER), DXFC_ELEMENT(NuamOrderType, IMBALANCE)});
    checkEnum<NuamTimeInForceType>(
        {DXFC_ELEMENT(NuamTimeInForceType, UNDEFINED), DXFC_ELEMENT(NuamTimeInForceType, REST_OF_DAY),
         DXFC_ELEMENT(NuamTimeInForceType, GTC), DXFC_ELEMENT(NuamTimeInForceType, IMMEDIATE_OR_CANCEL),
         DXFC_ELEMENT(NuamTimeInForceType, FILL_OR_KILL), DXFC_ELEMENT(NuamTimeInForceType, GOOD_TIL_SESSION),
         DXFC_ELEMENT(NuamTimeInForceType, NUMBER_OF_DAYS)});
    checkEnum<OrderAction>(
        {DXFC_ELEMENT(OrderAction, UNDEFINED), DXFC_ELEMENT(OrderAction, NEW), DXFC_ELEMENT(OrderAction, REPLACE),
         DXFC_ELEMENT(OrderAction, MODIFY), DXFC_ELEMENT(OrderAction, DELETE), DXFC_ELEMENT(OrderAction, PARTIAL),
         DXFC_ELEMENT(OrderAction, EXECUTE), DXFC_ELEMENT(OrderAction, TRADE), DXFC_ELEMENT(OrderAction, BUST)});
    checkEnum<OtcMarketsPriceType>({DXFC_ELEMENT(OtcMarketsPriceType, UNPRICED),
                                    DXFC_ELEMENT(OtcMarketsPriceType, ACTUAL),
                                    DXFC_ELEMENT(OtcMarketsPriceType, WANTED)});
    checkEnum<PriceType>({DXFC_ELEMENT(PriceType, REGULAR), DXFC_ELEMENT(PriceType, INDICATIVE),
                          DXFC_ELEMENT(PriceType, PRELIMINARY), DXFC_ELEMENT(PriceType, FINAL)});
    checkEnum<ShortSaleRestriction>({DXFC_ELEMENT(ShortSaleRestriction, UNDEFINED),
                                     DXFC_ELEMENT(ShortSaleRestriction, ACTIVE),
                                     DXFC_ELEMENT(ShortSaleRestriction, INACTIVE)});
    checkEnum<TimeAndSaleType>({DXFC_ELEMENT(TimeAndSaleType, NEW), DXFC_ELEMENT(TimeAndSaleType, CORRECTION),
                                DXFC_ELEMENT(TimeAndSaleType, CANCEL)});
    checkEnum<TradingStatus>({DXFC_ELEMENT(TradingStatus, UNDEFINED), DXFC_ELEMENT(TradingStatus, HALTED),
                              DXFC_ELEMENT(TradingStatus, ACTIVE)});
    checkEnum<InstrumentProfileType>(
        {DXFC_ELEMENT(InstrumentProfileType, CURRENCY), DXFC_ELEMENT(InstrumentProfileType, FOREX),
         DXFC_ELEMENT(InstrumentProfileType, BOND), DXFC_ELEMENT(InstrumentProfileType, INDEX),
         DXFC_ELEMENT(InstrumentProfileType, STOCK), DXFC_ELEMENT(InstrumentProfileType, ETF),
         DXFC_ELEMENT(InstrumentProfileType, MUTUAL_FUND), DXFC_ELEMENT(InstrumentProfileType, MONEY_MARKET_FUND),
         DXFC_ELEMENT(InstrumentProfileType, PRODUCT), DXFC_ELEMENT(InstrumentProfileType, FUTURE),
         DXFC_ELEMENT(InstrumentProfileType, OPTION), DXFC_ELEMENT(InstrumentProfileType, WARRANT),
         DXFC_ELEMENT(InstrumentProfileType, CFD), DXFC_ELEMENT(InstrumentProfileType, SPREAD),
         DXFC_ELEMENT(InstrumentProfileType, OTHER), DXFC_ELEMENT(InstrumentProfileType, REMOVED)});
}

TEST_CASE("Enum::valueOf returns the element of code 0 for an unknown code") {
    CHECK(&Side::valueOf(100) == &Side::UNDEFINED);
    CHECK(&Scope::valueOf(100) == &Scope::COMPOSITE);
    // The codes of the flag bits that no element has (the codes of Java enums are 0..7).
    CHECK(&ImbalanceSide::valueOf(6) == &ImbalanceSide::UNDEFINED);
    CHECK(&ImbalanceSide::valueOf(7) == &ImbalanceSide::UNDEFINED);
    CHECK(&AuctionType::valueOf(5) == &AuctionType::UNDEFINED);
    CHECK(&AuctionType::valueOf(6) == &AuctionType::UNDEFINED);
    CHECK(&AuctionType::valueOf(7) == &AuctionType::UNDEFINED);
    // The codes of the flag bits (mask 15) that no element has.
    for (std::uint32_t code = 6; code <= 15; ++code) {
        CAPTURE(code);
        CHECK(&NuamOrderType::valueOf(code) == &NuamOrderType::UNDEFINED);
    }

    for (std::uint32_t code = 7; code <= 15; ++code) {
        CAPTURE(code);
        CHECK(&NuamTimeInForceType::valueOf(code) == &NuamTimeInForceType::UNDEFINED);
    }
}

TEST_CASE("The event types are the constants and are found by name, class name and id") {
    REQUIRE(EventTypeEnum::ALL.size() == EventTypeEnum::ALL_BY_NAME.size());
    REQUIRE(EventTypeEnum::ALL.size() == EventTypeEnum::ALL_BY_CLASS_NAME.size());
    REQUIRE(EventTypeEnum::ALL.size() == EventTypeEnum::ALL_BY_ID.size());

    for (const auto &eventTypeRef : EventTypeEnum::ALL) {
        const auto &eventType = eventTypeRef.get();

        CHECK(&EventTypeEnum::ALL_BY_NAME.at(eventType.getName()).get() == &eventType);
        CHECK(&EventTypeEnum::ALL_BY_CLASS_NAME.at(eventType.getClassName()).get() == &eventType);
        CHECK(&EventTypeEnum::ALL_BY_ID.at(eventType.getId()).get() == &eventType);
    }

    CHECK(&EventTypeEnum::ALL.front().get() == &EventTypeEnum::QUOTE);
    CHECK(EventTypeEnum::QUOTE.getName() == "QUOTE");
    CHECK(EventTypeEnum::QUOTE.getClassName() == "Quote");
    CHECK(EventTypeEnum::TIME_AND_SALE.getName() == "TIME_AND_SALE");
    CHECK(EventTypeEnum::TIME_AND_SALE.getClassName() == "TimeAndSale");
    CHECK("TIME_AND_SALE"_et == EventTypeEnum::TIME_AND_SALE);
    CHECK("UNKNOWN"_et == EventTypeEnum::INVALID_EVENT_TYPE);
}

TEST_CASE("The event flags keep their values and names") {
    const std::vector<std::pair<std::reference_wrapper<const EventFlag>, std::pair<std::uint32_t, std::string>>>
        expected{
            {std::cref(EventFlag::TX_PENDING), {0x01u, "TX_PENDING"}},
            {std::cref(EventFlag::REMOVE_EVENT), {0x02u, "REMOVE_EVENT"}},
            {std::cref(EventFlag::SNAPSHOT_BEGIN), {0x04u, "SNAPSHOT_BEGIN"}},
            {std::cref(EventFlag::SNAPSHOT_END), {0x08u, "SNAPSHOT_END"}},
            {std::cref(EventFlag::SNAPSHOT_SNIP), {0x10u, "SNAPSHOT_SNIP"}},
            {std::cref(EventFlag::SNAPSHOT_MODE), {0x40u, "SNAPSHOT_MODE"}},
            {std::cref(EventFlag::REMOVE_SYMBOL), {0x80u, "REMOVE_SYMBOL"}},
            {std::cref(IndexedEvent::TX_PENDING), {0x01u, "TX_PENDING"}},
            {std::cref(IndexedEvent::REMOVE_EVENT), {0x02u, "REMOVE_EVENT"}},
            {std::cref(IndexedEvent::SNAPSHOT_BEGIN), {0x04u, "SNAPSHOT_BEGIN"}},
            {std::cref(IndexedEvent::SNAPSHOT_END), {0x08u, "SNAPSHOT_END"}},
            {std::cref(IndexedEvent::SNAPSHOT_SNIP), {0x10u, "SNAPSHOT_SNIP"}},
            {std::cref(IndexedEvent::SNAPSHOT_MODE), {0x40u, "SNAPSHOT_MODE"}},
        };

    for (const auto &[flagRef, value] : expected) {
        CHECK(flagRef.get().getFlag() == value.first);
        CHECK(flagRef.get().getName() == value.second);
    }
}

TEST_CASE("The candle attribute collections are their constants, which are parsed back") {
    checkSameConstants<CandleType>(
        CandleType::VALUES,
        {DXFC_ELEMENT(CandleType, TICK), DXFC_ELEMENT(CandleType, SECOND), DXFC_ELEMENT(CandleType, MINUTE),
         DXFC_ELEMENT(CandleType, HOUR), DXFC_ELEMENT(CandleType, DAY), DXFC_ELEMENT(CandleType, WEEK),
         DXFC_ELEMENT(CandleType, MONTH), DXFC_ELEMENT(CandleType, OPTEXP), DXFC_ELEMENT(CandleType, YEAR),
         DXFC_ELEMENT(CandleType, VOLUME), DXFC_ELEMENT(CandleType, PRICE), DXFC_ELEMENT(CandleType, PRICE_MOMENTUM),
         DXFC_ELEMENT(CandleType, PRICE_RENKO)});

    for (const auto &typeRef : CandleType::VALUES) {
        const auto &type = typeRef.get();

        CHECK(&CandleType::parse(type.toString()).get() == &type);
        CHECK(&CandleType::parse(type.getName()).get() == &type);
        CHECK(&CandleType::BY_STRING.at(type.toString()).get() == &type);
    }

    CHECK(CandleType::MONTH.toString() == "mo");
    CHECK(CandleType::MINUTE.getName() == "MINUTE");
    CHECK(CandleType::DAY.getPeriodIntervalMillis() == 24LL * 60LL * 60LL * 1000LL);
    CHECK(&CandleType::parse("Minutes").get() == &CandleType::MINUTE);

    checkSameConstants<CandlePrice>(CandlePrice::VALUES,
                                    {DXFC_ELEMENT(CandlePrice, LAST), DXFC_ELEMENT(CandlePrice, BID),
                                     DXFC_ELEMENT(CandlePrice, ASK), DXFC_ELEMENT(CandlePrice, MARK),
                                     DXFC_ELEMENT(CandlePrice, SETTLEMENT)});

    for (const auto &priceRef : CandlePrice::VALUES) {
        CHECK(&CandlePrice::parse(priceRef.get().toString()).get() == &priceRef.get());
        CHECK(&CandlePrice::BY_STRING.at(priceRef.get().toString()).get() == &priceRef.get());
    }

    checkSameConstants<CandleAlignment>(
        CandleAlignment::VALUES, {DXFC_ELEMENT(CandleAlignment, MIDNIGHT), DXFC_ELEMENT(CandleAlignment, SESSION)});

    for (const auto &alignmentRef : CandleAlignment::VALUES) {
        CHECK(&CandleAlignment::parse(alignmentRef.get().toString()).get() == &alignmentRef.get());
        CHECK(&CandleAlignment::BY_STRING.at(alignmentRef.get().toString()).get() == &alignmentRef.get());
    }

    checkSameConstants<CandleSession>(CandleSession::VALUES,
                                      {DXFC_ELEMENT(CandleSession, ANY), DXFC_ELEMENT(CandleSession, REGULAR)});

    for (const auto &sessionRef : CandleSession::VALUES) {
        CHECK(&CandleSession::parse(sessionRef.get().toString()).get() == &sessionRef.get());
        CHECK(&CandleSession::BY_STRING.at(sessionRef.get().toString()).get() == &sessionRef.get());
    }
}

TEST_CASE("The candle attributes are parsed by a case-insensitive prefix of their strings") {
    CHECK(&CandlePrice::parse("BI").get() == &CandlePrice::BID);
    CHECK(&CandleAlignment::parse("S").get() == &CandleAlignment::SESSION);
    CHECK(&CandleSession::parse("TRUE").get() == &CandleSession::REGULAR);
    CHECK(&CandleSession::parse("t").get() == &CandleSession::REGULAR);
    CHECK(&CandleSession::parse("F").get() == &CandleSession::ANY);

    CHECK_THROWS_AS(CandlePrice::parse(""), InvalidArgumentException);
    CHECK_THROWS_AS(CandlePrice::parse("zzz"), InvalidArgumentException);
    CHECK_THROWS_AS(CandleAlignment::parse(""), InvalidArgumentException);
    CHECK_THROWS_AS(CandleAlignment::parse("zzz"), InvalidArgumentException);
    CHECK_THROWS_AS(CandleSession::parse(""), InvalidArgumentException);
    CHECK_THROWS_AS(CandleSession::parse("zzz"), InvalidArgumentException);
}

TEST_CASE("The constants that repeat the data of other constants have the same data") {
    CHECK(CandlePeriod::TICK.getType().getName() == CandleType::TICK.getName());
    CHECK(CandlePeriod::DAY.getType().getName() == CandleType::DAY.getName());
    CHECK(CandlePeriod::DAY.getType().getPeriodIntervalMillis() == CandleType::DAY.getPeriodIntervalMillis());
    CHECK(CandlePeriod::DAY.getPeriodIntervalMillis() == CandleType::DAY.getPeriodIntervalMillis());
    CHECK(CandlePeriod::DEFAULT.getType().getName() == CandleType::TICK.getName());

    const std::pair<const SessionFilter *, const SessionType *> filters[] = {
        {&SessionFilter::NO_TRADING, &SessionType::NO_TRADING},
        {&SessionFilter::PRE_MARKET, &SessionType::PRE_MARKET},
        {&SessionFilter::REGULAR, &SessionType::REGULAR},
        {&SessionFilter::AFTER_MARKET, &SessionType::AFTER_MARKET},
    };

    for (const auto &[filter, type] : filters) {
        REQUIRE(filter->getType());
        CHECK(filter->getType()->getName() == type->getName());
        CHECK(filter->getType()->isTrading() == type->isTrading());
    }
}

TEST_CASE("The DEFAULT candle attributes equal the constants they are documented to be") {
    CHECK(CandleAlignment::DEFAULT == CandleAlignment::MIDNIGHT);
    CHECK(CandleAlignment::DEFAULT.toString() == "m");
    CHECK(CandleExchange::DEFAULT == CandleExchange::COMPOSITE);
    CHECK(CandleExchange::DEFAULT.getExchangeCode() == '\0');
    CHECK(CandlePrice::DEFAULT == CandlePrice::LAST);
    CHECK(CandlePrice::DEFAULT.toString() == "last");
    CHECK(CandleSession::DEFAULT == CandleSession::ANY);
    CHECK(&CandleSession::DEFAULT.getSessionFilter() == &SessionFilter::ANY);
    CHECK(&CandleSession::REGULAR.getSessionFilter() == &SessionFilter::REGULAR);
    CHECK(std::isnan(CandlePriceLevel::DEFAULT.getValue()));

    CHECK(CandlePeriod::DEFAULT == CandlePeriod::TICK);
    CHECK(CandlePeriod::TICK.toString() == "t");
    CHECK(CandlePeriod::TICK.getValue() == 1.0);
    CHECK(CandlePeriod::TICK.getType() == CandleType::TICK);
    CHECK(CandlePeriod::DAY.toString() == "d");
    CHECK(CandlePeriod::DAY.getType() == CandleType::DAY);
    CHECK(CandlePeriod::parse("t") == CandlePeriod::TICK);
    CHECK(CandlePeriod::parse("1d") == CandlePeriod::DAY);
    CHECK(CandlePeriod::valueOf(5, CandleType::MINUTE).toString() == "5m");

    // The keys are literals, usable in constant expressions.
    static_assert(std::string_view{CandleAlignment::ATTRIBUTE_KEY} == "a");
    static_assert(std::string_view{CandlePeriod::ATTRIBUTE_KEY}.empty());
    static_assert(std::string_view{CandlePrice::ATTRIBUTE_KEY} == "price");
    static_assert(std::string_view{CandlePriceLevel::ATTRIBUTE_KEY} == "pl");
    static_assert(std::string_view{CandleSession::ATTRIBUTE_KEY} == "tho");
}

TEST_CASE("The session types and filters keep their names and conditions") {
    CHECK(SessionType::NO_TRADING.getName() == "NO_TRADING");
    CHECK(SessionType::NO_TRADING.toString() == "NO_TRADING");
    CHECK(!SessionType::NO_TRADING.isTrading());
    CHECK(SessionType::REGULAR.getName() == "REGULAR");
    CHECK(SessionType::REGULAR.isTrading());

    CHECK(!SessionFilter::ANY.getType());
    CHECK(!SessionFilter::ANY.getTrading());
    CHECK(SessionFilter::TRADING.getTrading() == true);
    CHECK(SessionFilter::NON_TRADING.getTrading() == false);
    REQUIRE(SessionFilter::REGULAR.getType());
    CHECK(*SessionFilter::REGULAR.getType() == SessionType::REGULAR);
    CHECK(SessionFilter::REGULAR.getType()->getName() == "REGULAR");
    CHECK(!SessionFilter::REGULAR.getTrading());
    CHECK(*SessionFilter::AFTER_MARKET.getType() == SessionType::AFTER_MARKET);

    CHECK(DayFilter::WEEK_DAY.getName() == "WEEK_DAY");
    CHECK(DayFilter::WEEK_DAY.getDayOfWeekMask() == 62u);
}

namespace {

// Local constants: the address of a constant imported from the DLL is not a constant expression.
struct MapElement {
    int value;
};

constinit const MapElement ONE{1};
constinit const MapElement TWO{2};
constinit const MapElement THREE{3};

} // namespace

TEST_CASE("ConstantMap finds the constants by key") {
    static constexpr auto ENTRIES = ConstantMap<std::string_view, MapElement>::sorted({
        {"one", ONE},
        {"two", TWO},
        {"three", THREE},
    });
    static constexpr ConstantMap<std::string_view, MapElement> map{ENTRIES};

    static_assert(map.size() == 3);
    static_assert(ENTRIES[0].first == "one" && ENTRIES[1].first == "three" && ENTRIES[2].first == "two");
    static_assert(map.contains("two") && !map.contains("four") && !map.contains(""));
    static_assert(&map.at("three").get() == &THREE);

    CHECK(map.find("four") == map.end());
    CHECK(map.find("one") == map.begin());
    CHECK(map.at("two").get().value == 2);
    CHECK_THROWS_AS(static_cast<void>(map.at("four")), std::out_of_range);

    // A repeated key: at compile time it is a compilation error.
    CHECK_THROWS_AS(static_cast<void>(ConstantMap<std::string_view, MapElement>::sorted({
                        {"one", ONE},
                        {"one", TWO},
                    })),
                    std::invalid_argument);
}

TEST_CASE("The predefined order sources are found by id and by name") {
    CHECK(IndexedEventSource::DEFAULT.id() == 0);
    CHECK(IndexedEventSource::DEFAULT.name() == "DEFAULT");

    for (const auto &[key, sourceRef] : OrderSource::PREDEFINED_SOURCES) {
        const auto &source = sourceRef.get();

        // The keys are written apart from the sources.
        if (std::holds_alternative<std::int32_t>(key)) {
            CHECK(std::get<std::int32_t>(key) == source.id());
        } else {
            CHECK(std::get<std::string_view>(key) == source.name());
        }

        CHECK(&OrderSource::PREDEFINED_SOURCES.at(source.id()).get() == &source);
        CHECK(&OrderSource::PREDEFINED_SOURCES.at(source.name()).get() == &source);
        CHECK(&OrderSource::valueOf(source.id()) == &source);
        CHECK(&OrderSource::valueOf(source.name()) == &source);
    }

    CHECK(OrderSource::NTV.name() == "NTV");
    CHECK(OrderSource::NTV.id() == 0x4E5456); // "NTV"
    CHECK(OrderSource::COMPOSITE_BID.id() == 1);
}

TEST_CASE("OrderSource::valueOf returns the same user source to concurrent callers") {
    constexpr int ITERATIONS = 1000;
    const OrderSource *first = nullptr;
    const OrderSource *second = nullptr;
    bool same = true;

    std::thread thread([&] {
        for (int i = 0; i < ITERATIONS; i++) {
            const auto *source = &OrderSource::valueOf("CNST");

            if (first != nullptr && first != source) {
                same = false;
            }

            first = source;
        }
    });

    for (int i = 0; i < ITERATIONS; i++) {
        second = &OrderSource::valueOf(0x434E5354); // "CNST"
    }

    thread.join();

    CHECK(same);
    CHECK(first == second);
    CHECK(first->name() == "CNST");
}

TEST_CASE("WildcardSymbol::ALL and InstrumentProfileField keep their strings") {
    CHECK(WildcardSymbol::ALL.getSymbol() == "*");
    CHECK(WildcardSymbol::ALL.getSymbol() == WildcardSymbol::RESERVED_PREFIX);
    CHECK(WildcardSymbol::ALL.toString().find('*') != std::string::npos);

    CHECK(InstrumentProfileField::STRIKE.getName() == "STRIKE");
    CHECK(InstrumentProfileField::STRIKE.isNumericField());
    CHECK(!InstrumentProfileField::SYMBOL.isNumericField());
    REQUIRE(InstrumentProfileField::find("TRADING_HOURS"));
    CHECK(&InstrumentProfileField::find("TRADING_HOURS")->get() == &InstrumentProfileField::TRADING_HOURS);
    CHECK(!InstrumentProfileField::find("UNKNOWN"));
}

TEST_CASE("The DXEndpoint keys and the AuthToken constants keep their strings") {
    std::unordered_map<std::string, std::string> properties{};

    properties[DXEndpoint::DXFEED_ADDRESS_PROPERTY] = "demo.dxfeed.com:7300";

    CHECK(properties.at("dxfeed.address") == "demo.dxfeed.com:7300");
    static_assert(std::string_view{DXEndpoint::NAME_PROPERTY} == "name");

    // The keys of the Java DXEndpoint.
    const std::vector<std::pair<std::string, std::string>> keys{
        {DXEndpoint::NAME_PROPERTY, "name"},
        {DXEndpoint::DXFEED_PROPERTIES_PROPERTY, "dxfeed.properties"},
        {DXEndpoint::DXFEED_ADDRESS_PROPERTY, "dxfeed.address"},
        {DXEndpoint::DXFEED_USER_PROPERTY, "dxfeed.user"},
        {DXEndpoint::DXFEED_PASSWORD_PROPERTY, "dxfeed.password"},
        {DXEndpoint::DXFEED_THREAD_POOL_SIZE_PROPERTY, "dxfeed.threadPoolSize"},
        {DXEndpoint::DXFEED_AGGREGATION_PERIOD_PROPERTY, "dxfeed.aggregationPeriod"},
        {DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "dxfeed.wildcard.enable"},
        {DXEndpoint::DXPUBLISHER_PROPERTIES_PROPERTY, "dxpublisher.properties"},
        {DXEndpoint::DXPUBLISHER_ADDRESS_PROPERTY, "dxpublisher.address"},
        {DXEndpoint::DXPUBLISHER_THREAD_POOL_SIZE_PROPERTY, "dxpublisher.threadPoolSize"},
        {DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "dxendpoint.eventTime"},
        {DXEndpoint::DXENDPOINT_STORE_EVERYTHING_PROPERTY, "dxendpoint.storeEverything"},
        {DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY, "dxscheme.nanoTime"},
        {DXEndpoint::DXSCHEME_ENABLED_PROPERTY_PREFIX, "dxscheme.enabled."},
    };

    for (const auto &[key, expected] : keys) {
        CHECK(key == expected);
    }

    static_assert(std::string_view{AuthToken::BASIC_SCHEME} == "Basic");
    static_assert(std::string_view{AuthToken::BEARER_SCHEME} == "Bearer");
    static_assert(std::string_view{String::NUL} == "<null>");
    static_assert(std::string_view{String::EMPTY}.empty());
    CHECK(AuthToken::NULL_TOKEN.getHttpAuthorization() == String::NUL);
    const auto nullToken = AuthToken::createBasicTokenOrNull("", "");

    CHECK(nullToken == AuthToken::NULL_TOKEN);
}

TEST_CASE("The events without a symbol or a string return the same \"<null>\" objects") {
    const Candle candle{};
    const Profile profile{};

    CHECK(candle.getEventSymbol().toString() == String::NUL);
    CHECK(&candle.getEventSymbol() == &Candle{}.getEventSymbol());
    CHECK(profile.getDescription() == String::NUL);
    CHECK(profile.getStatusReason() == String::NUL);
    CHECK(&profile.getDescription() == &profile.getStatusReason());
}

TEST_CASE("The Graal-dependent constants are initialized once for concurrent first callers") {
    std::int64_t zeroTime = -1;
    const CFI *empty = nullptr;
    std::string emptyCode{};
    std::atomic<bool> start{};

    std::thread thread([&] {
        while (!start.load()) {
            std::this_thread::yield();
        }

        zeroTime = TimePeriod::ZERO.getTime();
        empty = CFI::EMPTY().get();
        emptyCode = CFI::EMPTY()->getCode();
    });

    start.store(true);

    const auto mainZeroTime = TimePeriod::ZERO.getTime();
    const auto *mainEmpty = CFI::EMPTY().get();
    const auto mainEmptyCode = CFI::EMPTY()->getCode();

    thread.join();

    CHECK(empty == mainEmpty);
    CHECK(zeroTime == 0);
    CHECK(mainZeroTime == 0);
    CHECK(emptyCode == "XXXXXX");
    CHECK(mainEmptyCode == "XXXXXX");
    CHECK(TimePeriod::UNLIMITED.getTime() == std::numeric_limits<std::int64_t>::max());
    CHECK(PriceIncrements::EMPTY()->getText().empty());
    CHECK(AdditionalUnderlyings::EMPTY()->getText().empty());
    CHECK(TimeFormat::GMT.format(0) == "0");
    // DEFAULT_WITH_MILLIS_WITH_TIMEZONE is built from DEFAULT_WITH_MILLIS, which is built from DEFAULT.
    CHECK(!TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(0).empty());
    CHECK(!TimeFormat::DEFAULT_WITH_MILLIS.format(0).empty());
    CHECK(!TimeFormat::DEFAULT.format(0).empty());
}

namespace {

std::atomic<int> lazyInitializerCalls{};

// The first call fails. The handle is empty: no Java object is created.
JavaObjectHandle<TimePeriod> lazyInitializer() {
    if (lazyInitializerCalls.fetch_add(1) == 0) {
        throw std::runtime_error("The first call fails");
    }

    return JavaObjectHandle<TimePeriod>{};
}

} // namespace

TEST_CASE("LazyJavaObjectHandle calls the initializer again after a failure and once after a success") {
    const LazyJavaObjectHandle<TimePeriod> handle{&lazyInitializer};

    CHECK(lazyInitializerCalls.load() == 0);
    CHECK_THROWS_AS(static_cast<void>(handle.get()), std::runtime_error);
    CHECK(lazyInitializerCalls.load() == 1);

    std::atomic<bool> start{};
    std::vector<std::thread> threads{};

    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&] {
            while (!start.load()) {
                std::this_thread::yield();
            }

            static_cast<void>(handle.get());
        });
    }

    start.store(true);
    static_cast<void>(handle.get());

    for (auto &thread : threads) {
        thread.join();
    }

    CHECK(lazyInitializerCalls.load() == 2);
    CHECK(&handle.get() == &handle.get());
}

// The internal functions are not exported from the shared library.
#ifndef DXFCPP_USE_DLLS
TEST_CASE("LazyJavaObjectHandle that is not a constant releases the Java object created by the initializer") {
    // As the EMPTY() objects of CFI, PriceIncrements and AdditionalUnderlyings.
    auto handle = std::make_unique<LazyJavaObjectHandle<TimePeriod>>(&isolated::util::IsolatedTimePeriod::ZERO);

    REQUIRE(static_cast<bool>(handle->get()));

    handle.reset();

    // A new Java handle is linked in the registry before the handles created earlier: if the destroyed handle stayed
    // there, the link writes to freed memory (AddressSanitizer).
    CHECK(TimePeriod::valueOf(1).getTime() == 1);
}
#endif

// The constants of these types are not destroyed at exit, so the destructors of global variables can use them.
static_assert(std::is_trivially_destructible_v<CandleAlignment>);
static_assert(std::is_trivially_destructible_v<CandleExchange>);
static_assert(std::is_trivially_destructible_v<CandlePeriod>);
static_assert(std::is_trivially_destructible_v<CandlePrice>);
static_assert(std::is_trivially_destructible_v<CandlePriceLevel>);
static_assert(std::is_trivially_destructible_v<CandleSession>);
static_assert(std::is_trivially_destructible_v<CandleType>);
static_assert(std::is_trivially_destructible_v<DayFilter>);
static_assert(std::is_trivially_destructible_v<EventTypeEnum>);
static_assert(std::is_trivially_destructible_v<SessionType>);
static_assert(std::is_trivially_destructible_v<WildcardSymbol>);

// The constants of these types are destroyed at exit without a change: the types are not polymorphic, the names of the
// constants are not owned, and the Java objects of the constants are not released (LazyJavaObjectHandle).
static_assert(!std::is_polymorphic_v<AuthToken>);
static_assert(!std::is_polymorphic_v<IndexedEventSource>);
static_assert(!std::is_polymorphic_v<OrderSource>);
static_assert(!std::is_polymorphic_v<SessionFilter>);
static_assert(!std::is_polymorphic_v<TimeFormat>);
static_assert(!std::is_polymorphic_v<TimePeriod>);

TEST_CASE("An order source keeps its Graal type without virtual functions, and a copy to IndexedEventSource does not") {
    // Through a reference to the base class, as DXFeed::getIndexedEventsPromise() converts a source.
    const auto graalSource = [](const IndexedEventSource &source) {
        return EventSourceWrapper::fromGraal(source.toGraalUnique().get());
    };

    CHECK(graalSource(OrderSource::NTV).isOrderSource());
    CHECK(graalSource(OrderSource::valueOf("abcd")).isOrderSource());

    const OrderSource copy = OrderSource::NTV;

    CHECK(graalSource(copy).isOrderSource());
    CHECK(copy == OrderSource::NTV);

    const IndexedEventSource sliced = OrderSource::NTV;

    CHECK(graalSource(sliced).isIndexedEventSource());
    CHECK(sliced.id() == OrderSource::NTV.id());
    CHECK(sliced.name() == "NTV");

    IndexedEventSource assigned = IndexedEventSource::DEFAULT;

    assigned = OrderSource::NTV;

    CHECK(graalSource(assigned).isIndexedEventSource());
    CHECK(assigned.id() == OrderSource::NTV.id());
    CHECK(graalSource(IndexedEventSource::DEFAULT).isIndexedEventSource());
}
