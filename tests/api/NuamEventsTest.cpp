// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include "../support/TestSupport.hpp"
#include "../support/UtcTimeZone.hpp"
#include "EventsJavaParity.hpp"

using namespace dxfcpp;
using namespace std::literals;

namespace {

constexpr std::int64_t EVENT_TIME = 1'701'703'226'537LL;
constexpr std::int64_t TIME = 1'701'703'300'123LL;

template <typename E>
const E &findByName(const std::span<const std::reference_wrapper<const E>> all, const std::string &name) {
    for (const auto &element : all) {
        if (element.get().getName() == name) {
            return element.get();
        }
    }

    throw std::runtime_error("No element " + name);
}

const events_java_parity::TypeFlags &flagsOf(const std::string &name) {
    for (const auto &flags : events_java_parity::TYPE_FLAGS) {
        if (name == flags.name) {
            return flags;
        }
    }

    throw std::runtime_error("No flags of " + name);
}

template <typename T, std::size_t N> const T &caseNamed(const T (&cases)[N], const std::string &name) {
    for (const auto &javaCase : cases) {
        if (name == javaCase.name) {
            return javaCase;
        }
    }

    throw std::runtime_error("No case " + name);
}

// Two doubles are the same value: both NaN or equal (the comparison is done on the bits to keep -Wfloat-equal quiet
// and to tell 0.0 from -0.0 only where it matters, which is nowhere in these tests).
bool sameDouble(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) {
        return std::isnan(a) && std::isnan(b);
    }

    return !(a < b) && !(b < a);
}

// The event with the values of the case that the Java probe recorded (tests/api/EventsJavaParity.hpp).
NuamOrder makeFromJavaCase(const events_java_parity::NuamOrderCase &javaCase) {
    NuamOrder order{};

    if (javaCase.symbol != nullptr) {
        order.setEventSymbol(javaCase.symbol);
    }

    order.setEventTime(javaCase.eventTime);
    order.setEventFlags(javaCase.eventFlags);
    // The index first: the source takes the high bits of it.
    order.setIndex(javaCase.index);
    order.setSource(OrderSource::valueOf(javaCase.source));
    order.setTime(javaCase.time);
    order.setSequence(javaCase.sequence);
    order.setTimeNanoPart(javaCase.timeNanoPart);
    order.setAction(findByName(OrderAction::ALL, javaCase.action));
    order.setActionTime(javaCase.actionTime);
    order.setOrderId(javaCase.orderId);
    order.setAuxOrderId(javaCase.auxOrderId);
    order.setPrice(javaCase.price);
    order.setSize(javaCase.size);
    order.setExecutedSize(javaCase.executedSize);
    order.setCount(javaCase.count);
    order.setExchangeCode(static_cast<std::int16_t>(javaCase.exchangeCode));
    order.setOrderSide(findByName(Side::ALL, javaCase.orderSide));
    order.setScope(findByName(Scope::ALL, javaCase.scope));
    order.setTradeId(javaCase.tradeId);
    order.setTradePrice(javaCase.tradePrice);
    order.setTradeSize(javaCase.tradeSize);

    if (javaCase.marketMaker != nullptr) {
        order.setMarketMaker(javaCase.marketMaker);
    }

    order.setActorId(javaCase.actorId);
    order.setParticipantId(javaCase.participantId);
    order.setSubmitterId(javaCase.submitterId);
    order.setOnBehalfOfSubmitterId(javaCase.onBehalfOfSubmitterId);

    if (javaCase.clientOrderId != nullptr) {
        order.setClientOrderId(javaCase.clientOrderId);
    }

    if (javaCase.customerAccount != nullptr) {
        order.setCustomerAccount(javaCase.customerAccount);
    }

    if (javaCase.customerInfo != nullptr) {
        order.setCustomerInfo(javaCase.customerInfo);
    }

    if (javaCase.exchangeInfo != nullptr) {
        order.setExchangeInfo(javaCase.exchangeInfo);
    }

    order.setTimeInForce(findByName(NuamTimeInForceType::ALL, javaCase.timeInForce));
    order.setTimeInForceData(javaCase.timeInForceData);
    order.setTriggerOrderBookId(javaCase.triggerOrderBookId);
    order.setTriggerPrice(javaCase.triggerPrice);
    order.setTriggerSessionType(javaCase.triggerSessionType);
    order.setOrderType(findByName(NuamOrderType::ALL, javaCase.orderType));
    order.setOrderQuantity(javaCase.orderQuantity);
    order.setDisplayQuantity(javaCase.displayQuantity);
    order.setRefreshQuantity(javaCase.refreshQuantity);
    order.setLeavesQuantity(javaCase.leavesQuantity);
    order.setMatchedQuantity(javaCase.matchedQuantity);

    return order;
}

NuamTrade makeFromJavaCase(const events_java_parity::NuamTradeCase &javaCase) {
    NuamTrade trade{};

    if (javaCase.symbol != nullptr) {
        trade.setEventSymbol(javaCase.symbol);
    }

    trade.setEventTime(javaCase.eventTime);
    trade.setTime(javaCase.time);
    trade.setSequence(javaCase.sequence);
    trade.setTimeNanoPart(javaCase.timeNanoPart);
    trade.setExchangeCode(static_cast<std::int16_t>(javaCase.exchangeCode));
    // The tradeId of the Java class has no counterpart in the C++ Trade: the cases keep it 0.
    trade.setPrice(javaCase.price);
    trade.setChange(javaCase.change);
    trade.setSize(javaCase.size);
    trade.setDayId(javaCase.dayId);
    trade.setDayVolume(javaCase.dayVolume);
    trade.setDayTurnover(javaCase.dayTurnover);
    trade.setTickDirection(findByName(Direction::ALL, javaCase.direction));
    trade.setExtendedTradingHours(javaCase.extendedTradingHours);
    trade.setTradeStatTime(javaCase.tradeStatTime);
    trade.setLastSignificantPrice(javaCase.lastSignificantPrice);
    trade.setLastPriceForAll(javaCase.lastPriceForAll);
    trade.setNumberOfTrades(javaCase.numberOfTrades);
    trade.setVWAP(javaCase.vwap);

    return trade;
}

NuamTimeAndSale makeFromJavaCase(const events_java_parity::NuamTimeAndSaleCase &javaCase) {
    NuamTimeAndSale sale{};

    if (javaCase.symbol != nullptr) {
        sale.setEventSymbol(javaCase.symbol);
    }

    sale.setEventTime(javaCase.eventTime);
    sale.setEventFlags(javaCase.eventFlags);
    sale.setTime(javaCase.time);
    sale.setSequence(javaCase.sequence);
    sale.setTimeNanoPart(javaCase.timeNanoPart);
    sale.setExchangeCode(static_cast<std::int16_t>(javaCase.exchangeCode));
    // The tradeId of the Java class has no counterpart in the C++ TimeAndSale: the cases keep it 0.
    sale.setPrice(javaCase.price);
    sale.setSize(javaCase.size);
    sale.setBidPrice(javaCase.bidPrice);
    sale.setAskPrice(javaCase.askPrice);

    if (javaCase.exchangeSaleConditions != nullptr) {
        sale.setExchangeSaleConditions(javaCase.exchangeSaleConditions);
    }

    sale.setTradeThroughExempt(static_cast<char>(javaCase.tradeThroughExempt));
    sale.setAggressorSide(findByName(Side::ALL, javaCase.aggressorSide));
    sale.setSpreadLeg(javaCase.spreadLeg);
    sale.setExtendedTradingHours(javaCase.extendedTradingHours);
    sale.setValidTick(javaCase.validTick);
    sale.setType(findByName(TimeAndSaleType::ALL, javaCase.type));

    if (javaCase.buyer != nullptr) {
        sale.setBuyer(javaCase.buyer);
    }

    if (javaCase.seller != nullptr) {
        sale.setSeller(javaCase.seller);
    }

    sale.setMatchId(javaCase.matchId);

    return sale;
}

// Compares toString() of the event made from every case with what Java prints for it. The Java classes of NuamTrade
// and NuamTimeAndSale also print the tradeId of their Java base class, which the C++ classes do not have: it is
// removed from the Java text (the cases keep it 0).
template <typename Cases>
void checkToStringParity(const std::string &className, const Cases &cases, bool stripTradeId) {
    // The zone must be UTC here (UtcTimeZoneListener), otherwise every comparison below fails on the time text.
    REQUIRE(TimeFormat::DEFAULT_WITH_MILLIS.format(1'700'000'000'123LL) == "20231114-221320.123");

    for (const auto &javaCase : cases) {
        CAPTURE(javaCase.name);

        auto expected = std::string(javaCase.str);

        if (javaCase.symbol == nullptr) {
            // A null symbol is String::NUL (`<null>`) in the C++ API, Java prints `null`.
            const auto javaNull = className + "{null,";

            REQUIRE(expected.rfind(javaNull, 0) == 0);
            expected.replace(0, javaNull.size(), className + "{" + String::NUL + ",");
        }

        if (stripTradeId) {
            const std::string tradeId = ", tradeId=0";
            const auto position = expected.find(tradeId);

            REQUIRE(position != std::string::npos);
            expected.erase(position, tradeId.size());
        }

        CHECK(makeFromJavaCase(javaCase).toString() == expected);
    }
}

} // namespace

TEST_CASE("NuamOrder has the flags of the Java class") {
    const auto flags = flagsOf("NuamOrder");

    CHECK(NuamOrder::TYPE.isLasting() == flags.lasting);
    CHECK(NuamOrder::TYPE.isIndexed() == flags.indexed);
    CHECK(NuamOrder::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(NuamOrder::TYPE.isMarket() == flags.market);
}

TEST_CASE("NuamTrade has the flags of the Java class") {
    const auto flags = flagsOf("NuamTrade");

    CHECK(NuamTrade::TYPE.isLasting() == flags.lasting);
    CHECK(NuamTrade::TYPE.isIndexed() == flags.indexed);
    CHECK(NuamTrade::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(NuamTrade::TYPE.isMarket() == flags.market);
}

TEST_CASE("NuamTimeAndSale has the flags of the Java class") {
    const auto flags = flagsOf("NuamTimeAndSale");

    CHECK(NuamTimeAndSale::TYPE.isLasting() == flags.lasting);
    CHECK(NuamTimeAndSale::TYPE.isIndexed() == flags.indexed);
    CHECK(NuamTimeAndSale::TYPE.isTimeSeries() == flags.timeSeries);
    CHECK(NuamTimeAndSale::TYPE.isMarket() == flags.market);
}

TEST_CASE("NuamOrder::toString() prints the fields as the Java API does") {
    checkToStringParity("NuamOrder", events_java_parity::NUAM_ORDER, false);
}

TEST_CASE("NuamTrade::toString() prints the fields as the Java API does") {
    checkToStringParity("NuamTrade", events_java_parity::NUAM_TRADE, true);
}

TEST_CASE("NuamTimeAndSale::toString() prints the fields as the Java API does") {
    checkToStringParity("NuamTimeAndSale", events_java_parity::NUAM_TIME_AND_SALE, true);
}

namespace {

// Publishes events to a local hub and returns what a subscription received. Only the default scheme of the SDK is
// used, so the fields that it does not transfer (see the checks below) do not come back.
struct NuamHubFixture : test::LocalHubFixture {
    NuamHubFixture()
        : LocalHubFixture(
              std::unordered_map<std::string, std::string>{{DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY, "true"}}) {
    }

    template <typename E>
    std::vector<std::shared_ptr<E>> publishAndReceive(const std::vector<SymbolWrapper> &symbols,
                                                      const std::vector<std::shared_ptr<E>> &published) {
        std::vector<std::shared_ptr<E>> received{};
        const auto subscription = DXFeedSubscription::create({E::TYPE});

        for (const auto &symbol : symbols) {
            subscription->addSymbols(symbol);
        }

        subscription->template addEventListener<E>([&received](const auto &events) {
            for (auto &&event : events) {
                received.push_back(event);
            }
        });

        feed->attachSubscription(subscription);
        publishAndProcess(published);
        feed->detachSubscription(subscription);

        return received;
    }
};

// A different value in every field, the strings and exchangeInfo included.
std::shared_ptr<NuamOrder> makePopulatedOrder(const std::string &symbol) {
    auto order = std::make_shared<NuamOrder>(symbol);

    order->setEventTime(EVENT_TIME);
    order->setIndex(77);
    order->setSource(OrderSource::NUAM);
    order->setTime(TIME);
    order->setSequence(42);
    order->setTimeNanoPart(123456);
    order->setAction(OrderAction::NEW);
    order->setActionTime(TIME + 999);
    order->setOrderId(1234567890123LL);
    order->setAuxOrderId(987654321LL);
    order->setPrice(150.25);
    order->setSize(100.5);
    order->setExecutedSize(25.75);
    order->setCount(7);
    order->setExchangeCode('N');
    order->setOrderSide(Side::BUY);
    order->setScope(Scope::ORDER);
    order->setTradeId(5551212);
    order->setTradePrice(150.5);
    order->setTradeSize(10.5);
    order->setMarketMaker("MM01");
    order->setActorId(11);
    order->setParticipantId(22);
    order->setSubmitterId(33);
    order->setOnBehalfOfSubmitterId(44);
    order->setClientOrderId("CL-1_a");
    order->setCustomerAccount("ACC-9");
    order->setCustomerInfo("info:x");
    order->setExchangeInfo("XI-77");
    order->setTimeInForce(NuamTimeInForceType::GTC);
    order->setTimeInForceData(3);
    order->setTriggerOrderBookId(555);
    order->setTriggerPrice(149.5);
    order->setTriggerSessionType(2);
    order->setOrderType(NuamOrderType::LIMIT);
    order->setOrderQuantity(1000.5);
    order->setDisplayQuantity(100.25);
    order->setRefreshQuantity(50.5);
    order->setLeavesQuantity(850.0);
    order->setMatchedQuantity(150.5);

    return order;
}

std::shared_ptr<NuamTrade> makePopulatedTrade(const std::string &symbol) {
    auto trade = std::make_shared<NuamTrade>(symbol);

    trade->setEventTime(EVENT_TIME);
    trade->setTime(TIME);
    trade->setSequence(42);
    trade->setTimeNanoPart(123456);
    trade->setExchangeCode('Q');
    trade->setPrice(150.25);
    trade->setChange(1.25);
    trade->setSize(100.5);
    trade->setDayId(19675);
    trade->setDayVolume(123456.5);
    trade->setDayTurnover(9876543.25);
    trade->setTickDirection(Direction::UP);
    trade->setExtendedTradingHours(true);
    trade->setTradeStatTime(TIME - 1000);
    trade->setLastSignificantPrice(150.5);
    trade->setLastPriceForAll(150.125);
    trade->setNumberOfTrades(321);
    trade->setVWAP(149.875);

    return trade;
}

std::shared_ptr<NuamTimeAndSale> makePopulatedSale(const std::string &symbol) {
    auto sale = std::make_shared<NuamTimeAndSale>(symbol);

    sale->setEventTime(EVENT_TIME);
    sale->setTime(TIME);
    sale->setSequence(42);
    sale->setTimeNanoPart(123456);
    sale->setExchangeCode('T');
    sale->setPrice(150.25);
    sale->setSize(100.5);
    sale->setBidPrice(150.0);
    sale->setAskPrice(150.5);
    sale->setExchangeSaleConditions("TI");
    sale->setTradeThroughExempt('X');
    sale->setAggressorSide(Side::BUY);
    sale->setSpreadLeg(true);
    sale->setExtendedTradingHours(true);
    sale->setValidTick(true);
    sale->setType(TimeAndSaleType::CORRECTION);
    sale->setBuyer("BUYER-1");
    sale->setSeller("SELLER-2");
    sale->setMatchId(1234567890123LL);

    return sale;
}

} // namespace

TEST_CASE_FIXTURE(NuamHubFixture, "NuamOrder is published on NUAM and received with every field") {
    const auto published = makePopulatedOrder("AAPL");
    const auto received = publishAndReceive<NuamOrder>({"AAPL"}, {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "AAPL");
    CHECK(event.getEventTime() == EVENT_TIME);
    CHECK(event.getSource() == OrderSource::NUAM);
    CHECK(event.getIndex() == published->getIndex());
    CHECK(event.getTime() == TIME);
    CHECK(event.getSequence() == 42);
    CHECK(&event.getAction() == &OrderAction::NEW);
    CHECK(sameDouble(event.getPrice(), 150.25));
    CHECK(sameDouble(event.getSize(), 100.5));
    CHECK(event.getExchangeCode() == 'N');
    CHECK(&event.getOrderSide() == &Side::BUY);
    CHECK(&event.getScope() == &Scope::ORDER);
    CHECK(event.getActorId() == 11);
    CHECK(event.getParticipantId() == 22);
    CHECK(event.getSubmitterId() == 33);
    CHECK(event.getOnBehalfOfSubmitterId() == 44);
    CHECK(event.getClientOrderId() == "CL-1_a");
    CHECK(event.getCustomerAccount() == "ACC-9");
    CHECK(event.getCustomerInfo() == "info:x");
    CHECK(event.getExchangeInfo() == "XI-77");
    CHECK(&event.getTimeInForce() == &NuamTimeInForceType::GTC);
    CHECK(event.getTimeInForceData() == 3);
    CHECK(event.getTriggerOrderBookId() == 555);
    CHECK(sameDouble(event.getTriggerPrice(), 149.5));
    CHECK(event.getTriggerSessionType() == 2);
    CHECK(&event.getOrderType() == &NuamOrderType::LIMIT);
    CHECK(sameDouble(event.getOrderQuantity(), 1000.5));
    CHECK(sameDouble(event.getDisplayQuantity(), 100.25));
    CHECK(sameDouble(event.getRefreshQuantity(), 50.5));
    CHECK(sameDouble(event.getLeavesQuantity(), 850.0));
    CHECK(sameDouble(event.getMatchedQuantity(), 150.5));

    CHECK(received[0]->is<Order>());
    CHECK(std::dynamic_pointer_cast<Order>(received[0]) != nullptr);
}

TEST_CASE_FIXTURE(NuamHubFixture, "NuamOrder with null strings and NaN fields keeps them") {
    auto published = std::make_shared<NuamOrder>("MSFT");

    published->setSource(OrderSource::NUAM);
    published->setTime(TIME);
    published->setSequence(1);
    published->setOrderSide(Side::SELL);

    const auto received = publishAndReceive<NuamOrder>({"MSFT"}, {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "MSFT");
    CHECK(event.getSource() == OrderSource::NUAM);
    CHECK(event.getTime() == TIME);
    CHECK(&event.getOrderSide() == &Side::SELL);
    CHECK_FALSE(event.getClientOrderIdOpt().has_value());
    CHECK_FALSE(event.getCustomerAccountOpt().has_value());
    CHECK_FALSE(event.getCustomerInfoOpt().has_value());
    CHECK_FALSE(event.getExchangeInfoOpt().has_value());
    CHECK(std::isnan(event.getPrice()));
    CHECK(std::isnan(event.getSize()));
    CHECK(std::isnan(event.getTriggerPrice()));
    CHECK(std::isnan(event.getOrderQuantity()));
    CHECK(std::isnan(event.getDisplayQuantity()));
    CHECK(std::isnan(event.getRefreshQuantity()));
    CHECK(std::isnan(event.getLeavesQuantity()));
    CHECK(std::isnan(event.getMatchedQuantity()));
    CHECK(&event.getTimeInForce() == &NuamTimeInForceType::UNDEFINED);
    CHECK(&event.getOrderType() == &NuamOrderType::UNDEFINED);
}

TEST_CASE_FIXTURE(NuamHubFixture, "NuamTrade is published and received with every field, as a Trade") {
    const auto published = makePopulatedTrade("AAPL");
    const auto received = publishAndReceive<NuamTrade>({"AAPL"}, {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "AAPL");
    CHECK(event.getEventTime() == EVENT_TIME);
    CHECK(event.getTime() == TIME);
    CHECK(event.getSequence() == 42);
    CHECK(event.getExchangeCode() == 'Q');
    CHECK(sameDouble(event.getPrice(), 150.25));
    CHECK(sameDouble(event.getChange(), 1.25));
    CHECK(sameDouble(event.getSize(), 100.5));
    CHECK(event.getDayId() == 19675);
    CHECK(sameDouble(event.getDayVolume(), 123456.5));
    CHECK(sameDouble(event.getDayTurnover(), 9876543.25));
    CHECK(&event.getTickDirection() == &Direction::UP);
    CHECK(event.isExtendedTradingHours());
    CHECK(event.getTradeStatTime() == TIME - 1000);
    CHECK(sameDouble(event.getLastSignificantPrice(), 150.5));
    CHECK(sameDouble(event.getLastPriceForAll(), 150.125));
    CHECK(event.getNumberOfTrades() == 321);
    CHECK(sameDouble(event.getVWAP(), 149.875));

    // It is a Trade, not a TimeAndSale.
    CHECK(received[0]->is<NuamTrade>());
    CHECK(received[0]->is<Trade>());
    CHECK_FALSE(received[0]->is<TimeAndSale>());
    CHECK(std::dynamic_pointer_cast<Trade>(received[0]) != nullptr);
}

TEST_CASE_FIXTURE(NuamHubFixture, "NuamTimeAndSale is published and received with every field, as a TimeAndSale") {
    const auto published = makePopulatedSale("AAPL");
    const auto received =
        publishAndReceive<NuamTimeAndSale>({TimeSeriesSubscriptionSymbol("AAPL", TIME - 1000)}, {published});

    REQUIRE(received.size() == 1);

    const auto &event = *received[0];

    CHECK(event.getEventSymbol() == "AAPL");
    CHECK(event.getEventTime() == EVENT_TIME);
    CHECK(event.getTime() == TIME);
    CHECK(event.getSequence() == 42);
    CHECK(event.getExchangeCode() == 'T');
    CHECK(sameDouble(event.getPrice(), 150.25));
    CHECK(sameDouble(event.getSize(), 100.5));
    CHECK(sameDouble(event.getBidPrice(), 150.0));
    CHECK(sameDouble(event.getAskPrice(), 150.5));
    CHECK(event.getExchangeSaleConditions() == "TI");
    CHECK(event.getTradeThroughExempt() == 'X');
    CHECK(&event.getAggressorSide() == &Side::BUY);
    CHECK(event.isSpreadLeg());
    CHECK(event.isExtendedTradingHours());
    CHECK(event.isValidTick());
    CHECK(&event.getType() == &TimeAndSaleType::CORRECTION);
    CHECK(event.getMatchId() == 1234567890123LL);
    // Not compared: the buyer, the seller and the nano part of the time. The default scheme does not transfer them;
    // the assign() test below checks them.

    // It is a TimeAndSale, not a Trade.
    CHECK(received[0]->is<NuamTimeAndSale>());
    CHECK(received[0]->is<TimeAndSale>());
    CHECK_FALSE(received[0]->is<Trade>());
    CHECK(std::dynamic_pointer_cast<TimeAndSale>(received[0]) != nullptr);
}

TEST_CASE("assign() copies every field of NuamOrder") {
    const auto source =
        std::make_shared<NuamOrder>(makeFromJavaCase(caseNamed(events_java_parity::NUAM_ORDER, "full")));
    const auto target = std::make_shared<NuamOrder>();

    target->assign(source);

    CHECK(target->toString() == source->toString());
    CHECK(target->getEventSymbol() == source->getEventSymbol());
    CHECK(target->getEventTime() == source->getEventTime());
    CHECK(target->getSource() == source->getSource());
    CHECK(target->getIndex() == source->getIndex());
    CHECK(target->getMarketMaker() == source->getMarketMaker());
    CHECK(target->getClientOrderId() == source->getClientOrderId());
    CHECK(target->getCustomerAccount() == source->getCustomerAccount());
    CHECK(target->getCustomerInfo() == source->getCustomerInfo());
    CHECK(target->getExchangeInfo() == source->getExchangeInfo());
    CHECK(target->getExchangeInfo() == "XI-77");
    CHECK(target->getActorId() == 11);
    CHECK(target->getParticipantId() == 22);
    CHECK(target->getSubmitterId() == 33);
    CHECK(target->getOnBehalfOfSubmitterId() == 44);
    CHECK(&target->getTimeInForce() == &NuamTimeInForceType::GTC);
    CHECK(target->getTimeInForceData() == 3);
    CHECK(target->getTriggerOrderBookId() == 555);
    CHECK(sameDouble(target->getTriggerPrice(), 149.5));
    CHECK(target->getTriggerSessionType() == 2);
    CHECK(&target->getOrderType() == &NuamOrderType::LIMIT);
    CHECK(sameDouble(target->getOrderQuantity(), 1000.5));
    CHECK(sameDouble(target->getDisplayQuantity(), 100.25));
    CHECK(sameDouble(target->getRefreshQuantity(), 50.5));
    CHECK(sameDouble(target->getLeavesQuantity(), 850.0));
    CHECK(sameDouble(target->getMatchedQuantity(), 150.5));
}

TEST_CASE("assign() copies every field of NuamTrade") {
    const auto source =
        std::make_shared<NuamTrade>(makeFromJavaCase(caseNamed(events_java_parity::NUAM_TRADE, "full")));
    const auto target = std::make_shared<NuamTrade>();

    target->assign(source);

    CHECK(target->toString() == source->toString());
    CHECK(target->getEventSymbol() == "AAPL");
    CHECK(target->getTradeStatTime() == source->getTradeStatTime());
    CHECK(sameDouble(target->getLastSignificantPrice(), 150.5));
    CHECK(sameDouble(target->getLastPriceForAll(), 150.25));
    CHECK(target->getNumberOfTrades() == 321);
    CHECK(sameDouble(target->getVWAP(), 149.875));
    CHECK(target->getTimeNanoPart() == 123456);
    CHECK(sameDouble(target->getPrice(), 150.25));
    CHECK(sameDouble(target->getChange(), 1.25));
    CHECK(sameDouble(target->getDayVolume(), 123456.5));
    CHECK(&target->getTickDirection() == &Direction::UP);
    CHECK(target->isExtendedTradingHours());
}

TEST_CASE("assign() copies every field of NuamTimeAndSale") {
    const auto source =
        std::make_shared<NuamTimeAndSale>(makeFromJavaCase(caseNamed(events_java_parity::NUAM_TIME_AND_SALE, "full")));
    const auto target = std::make_shared<NuamTimeAndSale>();

    target->assign(source);

    CHECK(target->toString() == source->toString());
    CHECK(target->getEventSymbol() == "AAPL");
    CHECK(target->getMatchId() == 1234567890123LL);
    CHECK(target->getBuyer() == "BUYER-1");
    CHECK(target->getSeller() == "SELLER-2");
    CHECK(target->getTimeNanoPart() == 123456);
    CHECK(target->getExchangeSaleConditions() == "TI");
    CHECK(target->getTradeThroughExempt() == 'X');
    CHECK(&target->getType() == &TimeAndSaleType::CORRECTION);
    CHECK(target->isSpreadLeg());
    CHECK(target->isValidTick());
}
