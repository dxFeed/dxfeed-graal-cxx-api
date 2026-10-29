// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "EventFactory.hpp"

#include <array>
#include <cstdio>
#include <limits>

namespace dxfcpp::test {

namespace {

// 2023-11-14 22:13:20 UTC. Offsets below keep every time field distinct.
constexpr std::int64_t T = 1'700'000'000'000LL;
constexpr std::int32_t DAY_ID = 19'675; // 2023-11-14

} // namespace

std::vector<std::shared_ptr<EventType>> createTapeEvents() {
    std::vector<std::shared_ptr<EventType>> events{};

    auto quote = std::make_shared<Quote>(TAPE_SYMBOL);
    quote->setEventTime(T + 1);
    quote->setBidTime(T + 1'000);
    quote->setBidExchangeCode('B');
    quote->setBidPrice(10.25);
    quote->setBidSize(100);
    quote->setAskTime(T + 2'000);
    quote->setAskExchangeCode('A');
    quote->setAskPrice(10.5);
    quote->setAskSize(200);
    events.push_back(quote);

    auto trade = std::make_shared<Trade>(TAPE_SYMBOL);
    trade->setEventTime(T + 2);
    trade->setTime(T + 3'456);
    trade->setSequence(9);
    trade->setExchangeCode('Q');
    trade->setTradeId(7001);
    trade->setPrice(100.5);
    trade->setSize(10);
    trade->setTickDirection(Direction::UP);
    trade->setChange(1.25);
    trade->setDayId(DAY_ID);
    trade->setDayVolume(1'000);
    trade->setDayTurnover(100'500);
    trade->setExtendedTradingHours(true);
    events.push_back(trade);

    auto tradeEth = std::make_shared<TradeETH>(TAPE_SYMBOL);
    tradeEth->setEventTime(T + 3);
    tradeEth->setTime(T + 4'567);
    tradeEth->setSequence(11);
    tradeEth->setExchangeCode('P');
    tradeEth->setTradeId(7002);
    tradeEth->setPrice(101.75);
    tradeEth->setSize(20);
    tradeEth->setTickDirection(Direction::DOWN);
    tradeEth->setChange(-0.5);
    tradeEth->setDayId(DAY_ID + 1);
    tradeEth->setDayVolume(2'000);
    tradeEth->setDayTurnover(203'500);
    tradeEth->setExtendedTradingHours(true);
    events.push_back(tradeEth);

    auto timeAndSale = std::make_shared<TimeAndSale>(TAPE_SYMBOL);
    timeAndSale->setEventTime(T + 4);
    timeAndSale->setTime(T + 5'678);
    timeAndSale->setSequence(13);
    timeAndSale->setExchangeCode('T');
    timeAndSale->setTradeId(7003);
    timeAndSale->setPrice(102.5);
    timeAndSale->setSize(30);
    timeAndSale->setBidPrice(102.25);
    timeAndSale->setAskPrice(102.75);
    timeAndSale->setExchangeSaleConditions("TI");
    timeAndSale->setTradeThroughExempt('X');
    timeAndSale->setAggressorSide(Side::BUY);
    timeAndSale->setSpreadLeg(true);
    timeAndSale->setExtendedTradingHours(true);
    timeAndSale->setValidTick(true);
    timeAndSale->setType(TimeAndSaleType::CORRECTION);
    events.push_back(timeAndSale);

    auto summary = std::make_shared<Summary>(TAPE_SYMBOL);
    summary->setEventTime(T + 5);
    summary->setDayId(DAY_ID + 2);
    summary->setDayOpenPrice(90.5);
    summary->setDayHighPrice(95.25);
    summary->setDayLowPrice(89.75);
    summary->setDayClosePrice(94.5);
    summary->setDayClosePriceType(PriceType::FINAL);
    summary->setPrevDayId(DAY_ID + 1);
    summary->setPrevDayClosePrice(91.25);
    summary->setPrevDayClosePriceType(PriceType::PRELIMINARY);
    summary->setPrevDayVolume(3'000);
    summary->setOpenInterest(static_cast<std::int64_t>(4'000));
    events.push_back(summary);

    auto profile = std::make_shared<Profile>(TAPE_SYMBOL);
    profile->setEventTime(T + 6);
    profile->setBeta(1.5);
    profile->setEarningsPerShare(2.25);
    profile->setDividendFrequency(4);
    profile->setExDividendAmount(0.75);
    profile->setExDividendDayId(DAY_ID + 3);
    profile->setHigh52WeekPrice(120.5);
    profile->setLow52WeekPrice(80.25);
    profile->setShares(5'000'000);
    profile->setFreeFloat(4'000'000);
    profile->setHighLimitPrice(110.5);
    profile->setLowLimitPrice(85.5);
    profile->setHaltStartTime(T + 6'000);
    profile->setHaltEndTime(T + 7'000);
    profile->setShortSaleRestriction(ShortSaleRestriction::ACTIVE);
    profile->setTradingStatus(TradingStatus::HALTED);
    profile->setDescription("Round-trip test instrument");
    profile->setStatusReason("Test halt");
    events.push_back(profile);

    auto order = std::make_shared<Order>(TAPE_SYMBOL);
    order->setEventTime(T + 7);
    order->setIndex(21);
    order->setSource(OrderSource::NTV);
    order->setTime(T + 8'901);
    order->setSequence(15);
    order->setPrice(99.5);
    order->setSize(40);
    order->setOrderSide(Side::BUY);
    order->setScope(Scope::ORDER);
    order->setExchangeCode('N');
    order->setMarketMaker("MMKR");
    events.push_back(order);

    auto analyticOrder = std::make_shared<AnalyticOrder>(TAPE_SYMBOL);
    analyticOrder->setEventTime(T + 8);
    analyticOrder->setIndex(22);
    analyticOrder->setSource(OrderSource::GLBX);
    analyticOrder->setTime(T + 9'012);
    analyticOrder->setSequence(17);
    analyticOrder->setPrice(98.25);
    analyticOrder->setSize(50);
    analyticOrder->setOrderSide(Side::SELL);
    analyticOrder->setScope(Scope::ORDER);
    analyticOrder->setExchangeCode('G');
    events.push_back(analyticOrder);

    auto otcMarketsOrder = std::make_shared<OtcMarketsOrder>(TAPE_SYMBOL);
    otcMarketsOrder->setEventTime(T + 9);
    otcMarketsOrder->setIndex(23);
    otcMarketsOrder->setSource(OrderSource::pink);
    otcMarketsOrder->setTime(T + 10'123);
    otcMarketsOrder->setSequence(19);
    otcMarketsOrder->setPrice(0.0125);
    otcMarketsOrder->setSize(60);
    otcMarketsOrder->setOrderSide(Side::BUY);
    otcMarketsOrder->setScope(Scope::ORDER);
    otcMarketsOrder->setExchangeCode('U');
    otcMarketsOrder->setMarketMaker("OTCM");
    otcMarketsOrder->setQuoteAccessPayment(-3);
    otcMarketsOrder->setOpen(true);
    otcMarketsOrder->setUnsolicited(true);
    otcMarketsOrder->setOtcMarketsPriceType(OtcMarketsPriceType::ACTUAL);
    otcMarketsOrder->setSaturated(true);
    otcMarketsOrder->setAutoExecution(true);
    otcMarketsOrder->setNmsConditional(true);
    events.push_back(otcMarketsOrder);

    auto spreadOrder = std::make_shared<SpreadOrder>(TAPE_SYMBOL);
    spreadOrder->setEventTime(T + 10);
    spreadOrder->setIndex(24);
    spreadOrder->setSource(OrderSource::ISE);
    spreadOrder->setTime(T + 11'234);
    spreadOrder->setSequence(21);
    spreadOrder->setPrice(1.5);
    spreadOrder->setSize(70);
    spreadOrder->setOrderSide(Side::SELL);
    spreadOrder->setScope(Scope::ORDER);
    spreadOrder->setExchangeCode('I');
    spreadOrder->setSpreadSymbol("=ROUND-TRIP-SPREAD");
    events.push_back(spreadOrder);

    auto candle = std::make_shared<Candle>(CandleSymbol::valueOf(tapeCandleSymbol()));
    candle->setEventTime(T + 11);
    candle->setTime(T - 86'400'000);
    candle->setSequence(23);
    candle->setCount(1'234);
    candle->setOpen(10.5);
    candle->setHigh(12.25);
    candle->setLow(9.75);
    candle->setClose(11.5);
    candle->setVolume(5'000);
    candle->setVWAP(11.125);
    candle->setBidVolume(2'000);
    candle->setAskVolume(3'000);
    candle->setImpVolatility(0.25);
    candle->setOpenInterest(6'000.0);
    events.push_back(candle);

    auto greeks = std::make_shared<Greeks>(TAPE_SYMBOL);
    greeks->setEventTime(T + 12);
    greeks->setTime(T + 12'345);
    greeks->setSequence(25);
    greeks->setPrice(3.5);
    greeks->setVolatility(0.375);
    greeks->setDelta(0.5);
    greeks->setGamma(0.0625);
    greeks->setTheta(-0.125);
    greeks->setRho(0.25);
    greeks->setVega(0.75);
    events.push_back(greeks);

    auto theoPrice = std::make_shared<TheoPrice>(TAPE_SYMBOL);
    theoPrice->setEventTime(T + 13);
    theoPrice->setTime(T + 13'456);
    theoPrice->setSequence(27);
    theoPrice->setPrice(4.25);
    theoPrice->setUnderlyingPrice(100.5);
    theoPrice->setDelta(0.625);
    theoPrice->setGamma(0.03125);
    theoPrice->setDividend(0.5);
    theoPrice->setInterest(0.0375);
    events.push_back(theoPrice);

    auto underlying = std::make_shared<Underlying>(TAPE_SYMBOL);
    underlying->setEventTime(T + 14);
    underlying->setTime(T + 14'567);
    underlying->setSequence(29);
    underlying->setVolatility(0.3125);
    underlying->setFrontVolatility(0.28125);
    underlying->setBackVolatility(0.34375);
    underlying->setCallVolume(7'000);
    underlying->setPutVolume(3'500);
    underlying->setPutCallRatio(0.5);
    events.push_back(underlying);

    auto series = std::make_shared<Series>(TAPE_SYMBOL);
    series->setEventTime(T + 15);
    series->setIndex(31);
    series->setTime(T + 15'678);
    series->setSequence(33);
    series->setExpiration(DAY_ID + 30);
    series->setVolatility(0.40625);
    series->setCallVolume(8'000);
    series->setPutVolume(2'000);
    series->setPutCallRatio(0.25);
    series->setForwardPrice(101.25);
    series->setDividend(0.625);
    series->setInterest(0.04375);
    events.push_back(series);

    auto optionSale = std::make_shared<OptionSale>(TAPE_SYMBOL);
    optionSale->setEventTime(T + 16);
    optionSale->setIndex(35);
    optionSale->setTime(T + 16'789);
    optionSale->setSequence(37);
    optionSale->setExchangeCode('C');
    optionSale->setPrice(5.25);
    optionSale->setSize(80);
    optionSale->setBidPrice(5.125);
    optionSale->setAskPrice(5.375);
    optionSale->setExchangeSaleConditions("OS");
    optionSale->setTradeThroughExempt('Y');
    optionSale->setAggressorSide(Side::SELL);
    optionSale->setSpreadLeg(true);
    optionSale->setExtendedTradingHours(true);
    optionSale->setValidTick(true);
    optionSale->setType(TimeAndSaleType::CANCEL);
    optionSale->setUnderlyingPrice(100.75);
    optionSale->setVolatility(0.46875);
    optionSale->setDelta(-0.375);
    optionSale->setOptionSymbol(".ROUND-TRIP231117C100");
    events.push_back(optionSale);

    auto textMessage = std::make_shared<TextMessage>(TAPE_SYMBOL);
    textMessage->setEventTime(T + 17);
    textMessage->setTime(T + 17'890);
    textMessage->setSequence(39);
    textMessage->setText("Text with spaces, \"quotes\" and \\backslash");
    events.push_back(textMessage);

    auto message = std::make_shared<Message>(TAPE_SYMBOL, "attachment");
    message->setEventTime(T + 18);
    events.push_back(message);

    // OrderImbalance is not an order: its source is only the record suffix (OrderImbalance#NTV), so it comes twice to
    // show that two sources of one symbol stay apart.
    auto orderImbalanceNtv = std::make_shared<OrderImbalance>(TAPE_SYMBOL);
    orderImbalanceNtv->setEventTime(T + 19);
    orderImbalanceNtv->setSource(OrderSource::NTV);
    orderImbalanceNtv->setTime(T + 19'234);
    orderImbalanceNtv->setSequence(41);
    orderImbalanceNtv->setRefPrice(150.25);
    orderImbalanceNtv->setPairedSize(1'200);
    orderImbalanceNtv->setImbalanceSize(300);
    orderImbalanceNtv->setNearPrice(150.5);
    orderImbalanceNtv->setFarPrice(149.75);
    orderImbalanceNtv->setImbalanceSide(ImbalanceSide::BUY);
    orderImbalanceNtv->setAuctionType(AuctionType::OPENING);
    events.push_back(orderImbalanceNtv);

    auto orderImbalanceNuam = std::make_shared<OrderImbalance>(TAPE_SYMBOL);
    orderImbalanceNuam->setEventTime(T + 20);
    orderImbalanceNuam->setSource(OrderSource::NUAM);
    orderImbalanceNuam->setTime(T + 20'345);
    orderImbalanceNuam->setSequence(43);
    orderImbalanceNuam->setRefPrice(99.5);
    orderImbalanceNuam->setPairedSize(2'500);
    orderImbalanceNuam->setImbalanceSize(700.5);
    orderImbalanceNuam->setNearPrice(99.75);
    orderImbalanceNuam->setFarPrice(99.25);
    orderImbalanceNuam->setImbalanceSide(ImbalanceSide::SELL);
    orderImbalanceNuam->setAuctionType(AuctionType::CLOSING);
    events.push_back(orderImbalanceNuam);

    // The Nuam events; the order comes twice, on the NUAM and on the lowercase nuam source (they are different
    // sources).
    auto nuamOrder = std::make_shared<NuamOrder>(TAPE_SYMBOL);
    nuamOrder->setEventTime(T + 21);
    nuamOrder->setIndex(25);
    nuamOrder->setSource(OrderSource::NUAM);
    nuamOrder->setTime(T + 21'456);
    nuamOrder->setSequence(45);
    nuamOrder->setPrice(120.5);
    nuamOrder->setSize(300);
    nuamOrder->setExchangeCode('U');
    nuamOrder->setOrderSide(Side::BUY);
    nuamOrder->setScope(Scope::ORDER);
    nuamOrder->setActorId(101);
    nuamOrder->setParticipantId(102);
    nuamOrder->setSubmitterId(103);
    nuamOrder->setOnBehalfOfSubmitterId(104);
    nuamOrder->setClientOrderId("CL-ORD-1");
    nuamOrder->setCustomerAccount("ACC-0042");
    nuamOrder->setCustomerInfo("Customer info");
    nuamOrder->setExchangeInfo("Exchange info");
    nuamOrder->setTimeInForce(NuamTimeInForceType::GTC);
    nuamOrder->setTimeInForceData(5);
    nuamOrder->setTriggerOrderBookId(77);
    nuamOrder->setTriggerPrice(118.75);
    nuamOrder->setTriggerSessionType(3);
    nuamOrder->setOrderType(NuamOrderType::LIMIT);
    nuamOrder->setOrderQuantity(1'000);
    nuamOrder->setDisplayQuantity(300);
    nuamOrder->setRefreshQuantity(150);
    nuamOrder->setLeavesQuantity(700);
    nuamOrder->setMatchedQuantity(300);
    events.push_back(nuamOrder);

    auto nuamOrderLower = std::make_shared<NuamOrder>(TAPE_SYMBOL);
    nuamOrderLower->setEventTime(T + 22);
    nuamOrderLower->setIndex(26);
    nuamOrderLower->setSource(OrderSource::valueOf("nuam"));
    nuamOrderLower->setTime(T + 22'567);
    nuamOrderLower->setSequence(47);
    nuamOrderLower->setPrice(121.25);
    nuamOrderLower->setSize(310);
    nuamOrderLower->setExchangeCode('V');
    nuamOrderLower->setOrderSide(Side::SELL);
    nuamOrderLower->setScope(Scope::ORDER);
    nuamOrderLower->setActorId(201);
    nuamOrderLower->setParticipantId(202);
    nuamOrderLower->setSubmitterId(203);
    nuamOrderLower->setOnBehalfOfSubmitterId(204);
    nuamOrderLower->setClientOrderId("CL-ORD-2");
    nuamOrderLower->setCustomerAccount("ACC-0043");
    nuamOrderLower->setCustomerInfo("Lower info");
    nuamOrderLower->setExchangeInfo("Lower exchange");
    nuamOrderLower->setTimeInForce(NuamTimeInForceType::IMMEDIATE_OR_CANCEL);
    nuamOrderLower->setTimeInForceData(6);
    nuamOrderLower->setTriggerOrderBookId(78);
    nuamOrderLower->setTriggerPrice(119.5);
    nuamOrderLower->setTriggerSessionType(4);
    nuamOrderLower->setOrderType(NuamOrderType::MARKET_TO_LIMIT);
    nuamOrderLower->setOrderQuantity(2'000);
    nuamOrderLower->setDisplayQuantity(310);
    nuamOrderLower->setRefreshQuantity(155);
    nuamOrderLower->setLeavesQuantity(1'690);
    nuamOrderLower->setMatchedQuantity(310);
    events.push_back(nuamOrderLower);

    auto nuamTrade = std::make_shared<NuamTrade>(TAPE_SYMBOL);
    nuamTrade->setEventTime(T + 23);
    nuamTrade->setTime(T + 23'678);
    nuamTrade->setSequence(49);
    nuamTrade->setExchangeCode('Y');
    nuamTrade->setTradeId(7004);
    nuamTrade->setPrice(130.25);
    nuamTrade->setChange(-0.75);
    nuamTrade->setSize(15);
    nuamTrade->setDayId(DAY_ID);
    nuamTrade->setDayVolume(1'500);
    nuamTrade->setDayTurnover(195'375);
    nuamTrade->setTickDirection(Direction::DOWN);
    nuamTrade->setExtendedTradingHours(false);
    nuamTrade->setTradeStatTime(T + 23'800);
    nuamTrade->setLastSignificantPrice(130.5);
    nuamTrade->setLastPriceForAll(130.25);
    nuamTrade->setNumberOfTrades(321);
    nuamTrade->setVWAP(129.875);
    events.push_back(nuamTrade);

    auto nuamTimeAndSale = std::make_shared<NuamTimeAndSale>(TAPE_SYMBOL);
    nuamTimeAndSale->setEventTime(T + 24);
    nuamTimeAndSale->setTime(T + 24'789);
    nuamTimeAndSale->setSequence(51);
    nuamTimeAndSale->setExchangeCode('X');
    nuamTimeAndSale->setTradeId(7005);
    nuamTimeAndSale->setPrice(131.5);
    nuamTimeAndSale->setSize(25);
    nuamTimeAndSale->setBidPrice(131.25);
    nuamTimeAndSale->setAskPrice(131.75);
    nuamTimeAndSale->setExchangeSaleConditions("TI");
    nuamTimeAndSale->setTradeThroughExempt('Z');
    nuamTimeAndSale->setAggressorSide(Side::SELL);
    nuamTimeAndSale->setSpreadLeg(false);
    nuamTimeAndSale->setExtendedTradingHours(true);
    nuamTimeAndSale->setValidTick(true);
    nuamTimeAndSale->setType(TimeAndSaleType::CANCEL);
    nuamTimeAndSale->setMatchId(9'876'543'210LL);
    events.push_back(nuamTimeAndSale);

    // The default scheme transfers the bid and ask times of MarketMaker in seconds.
    auto marketMaker = std::make_shared<MarketMaker>(TAPE_SYMBOL);
    marketMaker->setEventTime(T + 25);
    marketMaker->setExchangeCode('Q');
    marketMaker->setMarketMaker("NSDQ");
    marketMaker->setBidTime(T + 25'000);
    marketMaker->setBidPrice(132.25);
    marketMaker->setBidSize(400);
    marketMaker->setBidCount(4);
    marketMaker->setAskTime(T + 26'000);
    marketMaker->setAskPrice(132.75);
    marketMaker->setAskSize(500);
    marketMaker->setAskCount(5);
    events.push_back(marketMaker);

    auto textConfiguration = std::make_shared<TextConfiguration>(TAPE_SYMBOL, "{\"limit\": 100}", 3);
    textConfiguration->setEventTime(T + 27);
    textConfiguration->setTime(T + 27'890);
    textConfiguration->setSequence(53);
    events.push_back(textConfiguration);

    return events;
}

std::vector<std::shared_ptr<EventType>> createDefaultEvents() {
    const auto order = std::make_shared<Order>(DEFAULTS_SYMBOL);
    const auto analyticOrder = std::make_shared<AnalyticOrder>(DEFAULTS_SYMBOL);
    const auto otcMarketsOrder = std::make_shared<OtcMarketsOrder>(DEFAULTS_SYMBOL);
    const auto spreadOrder = std::make_shared<SpreadOrder>(DEFAULTS_SYMBOL);

    order->setSource(OrderSource::NTV);
    analyticOrder->setSource(OrderSource::GLBX);
    otcMarketsOrder->setSource(OrderSource::pink);
    spreadOrder->setSource(OrderSource::ISE);

    const auto orderImbalance = std::make_shared<OrderImbalance>(DEFAULTS_SYMBOL);

    orderImbalance->setSource(OrderSource::NTV);

    const auto nuamOrder = std::make_shared<NuamOrder>(DEFAULTS_SYMBOL);

    nuamOrder->setSource(OrderSource::NUAM);

    std::vector<std::shared_ptr<EventType>> events{
        std::make_shared<Quote>(DEFAULTS_SYMBOL),
        std::make_shared<Trade>(DEFAULTS_SYMBOL),
        std::make_shared<TradeETH>(DEFAULTS_SYMBOL),
        std::make_shared<TimeAndSale>(DEFAULTS_SYMBOL),
        std::make_shared<Summary>(DEFAULTS_SYMBOL),
        std::make_shared<Profile>(DEFAULTS_SYMBOL),
        order,
        analyticOrder,
        otcMarketsOrder,
        spreadOrder,
        std::make_shared<Candle>(CandleSymbol::valueOf(std::string(DEFAULTS_SYMBOL) + "{=d}")),
        std::make_shared<Greeks>(DEFAULTS_SYMBOL),
        std::make_shared<TheoPrice>(DEFAULTS_SYMBOL),
        std::make_shared<Underlying>(DEFAULTS_SYMBOL),
        std::make_shared<Series>(DEFAULTS_SYMBOL),
        std::make_shared<OptionSale>(DEFAULTS_SYMBOL),
        std::make_shared<TextMessage>(DEFAULTS_SYMBOL),
        std::make_shared<Message>(DEFAULTS_SYMBOL),
        orderImbalance,
        nuamOrder,
        std::make_shared<NuamTrade>(DEFAULTS_SYMBOL),
        std::make_shared<NuamTimeAndSale>(DEFAULTS_SYMBOL),
        std::make_shared<MarketMaker>(DEFAULTS_SYMBOL),
        std::make_shared<TextConfiguration>(DEFAULTS_SYMBOL),
    };

    for (const auto &event : events) {
        event->setEventTime(T);
    }

    return events;
}

namespace {

// The value sets of createEdgeEvents(): the k-th symbol gets the k-th value of every array.
const std::array<std::string, 11> EDGE_SYMBOLS{"EDGE-NAN",      "EDGE-ZERO",  "EDGE-NEG-ZERO", "EDGE-INF",
                                               "EDGE-NEG-INF",  "EDGE-MAX",   "EDGE-LOWEST",   "EDGE-DENORM",
                                               "EDGE-FRACTION", "EDGE-LARGE", "EDGE-SMALL"};

const std::array<double, 11> EDGE_DOUBLES{std::numeric_limits<double>::quiet_NaN(),
                                          0.0,
                                          -0.0,
                                          std::numeric_limits<double>::infinity(),
                                          -std::numeric_limits<double>::infinity(),
                                          std::numeric_limits<double>::max(),
                                          std::numeric_limits<double>::lowest(),
                                          std::numeric_limits<double>::denorm_min(),
                                          0.1,
                                          1e15 + 0.5,
                                          -1.5e-7};

const std::array<std::int64_t, 11> EDGE_LONGS{0,
                                              0,
                                              -1,
                                              std::numeric_limits<std::int64_t>::max(),
                                              std::numeric_limits<std::int64_t>::min(),
                                              std::numeric_limits<std::int64_t>::max(),
                                              std::numeric_limits<std::int64_t>::min(),
                                              1,
                                              1'700'000'000'123LL,
                                              -1'700'000'000'123LL,
                                              std::int64_t{std::numeric_limits<std::int32_t>::max()} + 1};

const std::array<std::int32_t, 11> EDGE_INTS{0,
                                             0,
                                             -1,
                                             std::numeric_limits<std::int32_t>::max(),
                                             std::numeric_limits<std::int32_t>::min(),
                                             std::numeric_limits<std::int32_t>::max(),
                                             std::numeric_limits<std::int32_t>::min(),
                                             1,
                                             19'675,
                                             -19'675,
                                             65'536};

// Sequences must be in [0, MAX_SEQUENCE] (4194303).
const std::array<std::int32_t, 11> EDGE_SEQUENCES{0, 0, 1, 4'194'303, 4'194'303, 4'194'303, 0, 1, 2, 3, 4};

const std::array<std::string, 11> EDGE_STRINGS{"",
                                               "<null>",
                                               "\\NULL",
                                               "\xC3\x9C\xE2\x82\xAC\xF0\x9F\x98\x80", // U+00DC U+20AC U+1F600
                                               "tab\tnew\nline",
                                               std::string(300, 'x'),
                                               " spaces ",
                                               "\"quoted\"",
                                               "back\\slash",
                                               ",;=#",
                                               "ASCII"};

// QD short strings (market maker, sale conditions): at most 8 ASCII characters.
const std::array<std::string, 11> EDGE_SHORT_STRINGS{"",         "<null>", "\\NULL", "A",    "Z9",   "~",
                                                     " spaces ", "\"q\"",  "a\\b",   ",;=#", "ASCII"};

const std::array<char, 3> EDGE_CHARS{'\0', ' ', '~'};

template <typename T, std::size_t N> const T &pick(const std::array<const T *, N> &values, std::size_t k) {
    return *values[k % N];
}

} // namespace

std::vector<std::string> edgeSymbols() {
    return {EDGE_SYMBOLS.begin(), EDGE_SYMBOLS.end()};
}

std::vector<std::string> edgeCandleSymbols() {
    std::vector<std::string> result{};

    for (const auto &symbol : EDGE_SYMBOLS) {
        result.push_back(symbol + "{=d}");
    }

    return result;
}

std::vector<std::shared_ptr<EventType>> createEdgeEvents() {
    const std::array<const Side *, 3> sides{&Side::UNDEFINED, &Side::BUY, &Side::SELL};
    // Java rejects a non-empty order with the UNDEFINED side.
    const std::array<const Side *, 2> orderSides{&Side::BUY, &Side::SELL};
    const std::array<const Scope *, 4> scopes{&Scope::COMPOSITE, &Scope::REGIONAL, &Scope::AGGREGATE, &Scope::ORDER};
    const std::array<const Direction *, 6> directions{&Direction::UNDEFINED, &Direction::DOWN,    &Direction::ZERO_DOWN,
                                                      &Direction::ZERO,      &Direction::ZERO_UP, &Direction::UP};
    const std::array<const PriceType *, 4> priceTypes{&PriceType::REGULAR, &PriceType::INDICATIVE,
                                                      &PriceType::PRELIMINARY, &PriceType::FINAL};
    const std::array<const TimeAndSaleType *, 3> timeAndSaleTypes{&TimeAndSaleType::NEW, &TimeAndSaleType::CORRECTION,
                                                                  &TimeAndSaleType::CANCEL};
    const std::array<const ShortSaleRestriction *, 3> shortSaleRestrictions{
        &ShortSaleRestriction::UNDEFINED, &ShortSaleRestriction::ACTIVE, &ShortSaleRestriction::INACTIVE};
    const std::array<const TradingStatus *, 3> tradingStatuses{&TradingStatus::UNDEFINED, &TradingStatus::HALTED,
                                                               &TradingStatus::ACTIVE};
    const std::array<const OtcMarketsPriceType *, 3> otcMarketsPriceTypes{
        &OtcMarketsPriceType::UNPRICED, &OtcMarketsPriceType::ACTUAL, &OtcMarketsPriceType::WANTED};

    // The sources of the NuamOrder: NUAM, the lowercase nuam (another source) and the default one.
    const std::array<const OrderSource *, 3> nuamSources{&OrderSource::NUAM, &OrderSource::valueOf("nuam"),
                                                         &OrderSource::DEFAULT};

    const std::array<const OrderSource *, 4> imbalanceSources{&OrderSource::NTV, &OrderSource::NUAM, &OrderSource::ARCA,
                                                              &OrderSource::DEFAULT};
    const std::array<const ImbalanceSide *, 6> imbalanceSides{
        &ImbalanceSide::UNDEFINED,           &ImbalanceSide::BUY,   &ImbalanceSide::SELL, &ImbalanceSide::NO_IMBALANCE,
        &ImbalanceSide::INSUFFICIENT_ORDERS, &ImbalanceSide::PAUSED};
    const std::array<const AuctionType *, 5> auctionTypes{&AuctionType::UNDEFINED, &AuctionType::OPENING,
                                                          &AuctionType::CLOSING, &AuctionType::REOPENING,
                                                          &AuctionType::OTHER};

    std::vector<std::shared_ptr<EventType>> events{};

    for (std::size_t k = 0; k < EDGE_SYMBOLS.size(); k++) {
        const auto &symbol = EDGE_SYMBOLS[k];
        const auto d = EDGE_DOUBLES[k];
        const auto l = EDGE_LONGS[k];
        const auto i = EDGE_INTS[k];
        const auto sequence = EDGE_SEQUENCES[k];
        const auto &s = EDGE_STRINGS[k];
        const auto &shortString = EDGE_SHORT_STRINGS[k];
        const auto c = EDGE_CHARS[k % EDGE_CHARS.size()];
        const bool b = k % 2 == 0;
        const auto index = static_cast<std::int64_t>(k + 1);

        auto quote = std::make_shared<Quote>(symbol);
        quote->setBidTime(l);
        quote->setBidExchangeCode(c);
        quote->setBidPrice(d);
        quote->setBidSize(d);
        quote->setAskTime(l);
        quote->setAskExchangeCode(c);
        quote->setAskPrice(d);
        quote->setAskSize(d);
        events.push_back(quote);

        for (const std::shared_ptr<TradeBase> &trade :
             {std::shared_ptr<TradeBase>(std::make_shared<Trade>(symbol)),
              std::shared_ptr<TradeBase>(std::make_shared<TradeETH>(symbol))}) {
            trade->setTime(l);
            trade->setSequence(sequence);
            trade->setExchangeCode(c);
            trade->setPrice(d);
            trade->setSize(d);
            trade->setTickDirection(pick(directions, k));
            trade->setChange(d);
            trade->setDayId(i);
            trade->setDayVolume(d);
            trade->setDayTurnover(d);
            trade->setExtendedTradingHours(b);
            events.push_back(trade);
        }

        auto timeAndSale = std::make_shared<TimeAndSale>(symbol);
        timeAndSale->setTime(l);
        timeAndSale->setSequence(sequence);
        timeAndSale->setExchangeCode(c);
        timeAndSale->setPrice(d);
        timeAndSale->setSize(d);
        timeAndSale->setBidPrice(d);
        timeAndSale->setAskPrice(d);
        timeAndSale->setExchangeSaleConditions(shortString);
        timeAndSale->setTradeThroughExempt(c);
        timeAndSale->setAggressorSide(pick(sides, k));
        timeAndSale->setSpreadLeg(b);
        timeAndSale->setExtendedTradingHours(!b);
        timeAndSale->setValidTick(b);
        timeAndSale->setType(pick(timeAndSaleTypes, k));
        events.push_back(timeAndSale);

        auto summary = std::make_shared<Summary>(symbol);
        summary->setDayId(i);
        summary->setDayOpenPrice(d);
        summary->setDayHighPrice(d);
        summary->setDayLowPrice(d);
        summary->setDayClosePrice(d);
        summary->setDayClosePriceType(pick(priceTypes, k));
        summary->setPrevDayId(i);
        summary->setPrevDayClosePrice(d);
        summary->setPrevDayClosePriceType(pick(priceTypes, k + 1));
        summary->setPrevDayVolume(d);
        summary->setOpenInterest(l);
        events.push_back(summary);

        auto profile = std::make_shared<Profile>(symbol);
        profile->setBeta(d);
        profile->setEarningsPerShare(d);
        profile->setDividendFrequency(d);
        profile->setExDividendAmount(d);
        profile->setExDividendDayId(i);
        profile->setHigh52WeekPrice(d);
        profile->setLow52WeekPrice(d);
        profile->setShares(d);
        profile->setFreeFloat(d);
        profile->setHighLimitPrice(d);
        profile->setLowLimitPrice(d);
        profile->setHaltStartTime(l);
        profile->setHaltEndTime(l);
        profile->setShortSaleRestriction(pick(shortSaleRestrictions, k));
        profile->setTradingStatus(pick(tradingStatuses, k));
        profile->setDescription(s);
        profile->setStatusReason(s);
        events.push_back(profile);

        auto order = std::make_shared<Order>(symbol);
        auto analyticOrder = std::make_shared<AnalyticOrder>(symbol);
        auto otcMarketsOrder = std::make_shared<OtcMarketsOrder>(symbol);
        auto spreadOrder = std::make_shared<SpreadOrder>(symbol);
        const std::array<std::shared_ptr<OrderBase>, 4> orders{order, analyticOrder, otcMarketsOrder, spreadOrder};
        const std::array<const OrderSource *, 4> sources{&OrderSource::NTV, &OrderSource::GLBX, &OrderSource::pink,
                                                         &OrderSource::ISE};

        for (std::size_t j = 0; j < orders.size(); j++) {
            const auto &orderBase = orders[j];

            orderBase->setIndex(index);
            orderBase->setSource(*sources[j]);
            orderBase->setTime(l);
            orderBase->setSequence(sequence);
            orderBase->setPrice(d);
            orderBase->setSize(d);
            orderBase->setOrderSide(pick(orderSides, k));
            orderBase->setScope(pick(scopes, k));
            orderBase->setExchangeCode(c);
            events.push_back(orderBase);
        }

        order->setMarketMaker(shortString);
        otcMarketsOrder->setMarketMaker(shortString);
        otcMarketsOrder->setQuoteAccessPayment(i);
        otcMarketsOrder->setOpen(b);
        otcMarketsOrder->setUnsolicited(!b);
        otcMarketsOrder->setOtcMarketsPriceType(pick(otcMarketsPriceTypes, k));
        otcMarketsOrder->setSaturated(b);
        otcMarketsOrder->setAutoExecution(!b);
        otcMarketsOrder->setNmsConditional(b);
        spreadOrder->setSpreadSymbol(s);

        auto candle = std::make_shared<Candle>(CandleSymbol::valueOf(symbol + "{=d}"));
        candle->setTime(l);
        candle->setSequence(sequence);
        candle->setCount(l);
        candle->setOpen(d);
        candle->setHigh(d);
        candle->setLow(d);
        candle->setClose(d);
        candle->setVolume(d);
        candle->setVWAP(d);
        candle->setBidVolume(d);
        candle->setAskVolume(d);
        candle->setImpVolatility(d);
        candle->setOpenInterest(d);
        events.push_back(candle);

        auto greeks = std::make_shared<Greeks>(symbol);
        greeks->setTime(l);
        greeks->setSequence(sequence);
        greeks->setPrice(d);
        greeks->setVolatility(d);
        greeks->setDelta(d);
        greeks->setGamma(d);
        greeks->setTheta(d);
        greeks->setRho(d);
        greeks->setVega(d);
        events.push_back(greeks);

        auto theoPrice = std::make_shared<TheoPrice>(symbol);
        theoPrice->setTime(l);
        theoPrice->setSequence(sequence);
        theoPrice->setPrice(d);
        theoPrice->setUnderlyingPrice(d);
        theoPrice->setDelta(d);
        theoPrice->setGamma(d);
        theoPrice->setDividend(d);
        theoPrice->setInterest(d);
        events.push_back(theoPrice);

        auto underlying = std::make_shared<Underlying>(symbol);
        underlying->setTime(l);
        underlying->setSequence(sequence);
        underlying->setVolatility(d);
        underlying->setFrontVolatility(d);
        underlying->setBackVolatility(d);
        underlying->setCallVolume(d);
        underlying->setPutVolume(d);
        underlying->setPutCallRatio(d);
        events.push_back(underlying);

        auto series = std::make_shared<Series>(symbol);
        series->setIndex(index);
        series->setTime(l);
        series->setSequence(sequence);
        series->setExpiration(i);
        series->setVolatility(d);
        series->setCallVolume(d);
        series->setPutVolume(d);
        series->setPutCallRatio(d);
        series->setForwardPrice(d);
        series->setDividend(d);
        series->setInterest(d);
        events.push_back(series);

        auto optionSale = std::make_shared<OptionSale>(symbol);
        optionSale->setIndex(index);
        optionSale->setTime(l);
        optionSale->setSequence(sequence);
        optionSale->setExchangeCode(c);
        optionSale->setPrice(d);
        optionSale->setSize(d);
        optionSale->setBidPrice(d);
        optionSale->setAskPrice(d);
        optionSale->setExchangeSaleConditions(shortString);
        optionSale->setTradeThroughExempt(c);
        optionSale->setAggressorSide(pick(sides, k));
        optionSale->setSpreadLeg(b);
        optionSale->setExtendedTradingHours(!b);
        optionSale->setValidTick(b);
        optionSale->setType(pick(timeAndSaleTypes, k));
        optionSale->setUnderlyingPrice(d);
        optionSale->setVolatility(d);
        optionSale->setDelta(d);
        optionSale->setOptionSymbol(s);
        events.push_back(optionSale);

        auto textMessage = std::make_shared<TextMessage>(symbol);
        textMessage->setTime(l);
        textMessage->setSequence(sequence);
        textMessage->setText(s);
        events.push_back(textMessage);

        events.push_back(std::make_shared<Message>(symbol, s));

        // The sources cycle through all the publishable ones, including the default (the record without a suffix).
        auto orderImbalance = std::make_shared<OrderImbalance>(symbol);

        orderImbalance->setSource(pick(imbalanceSources, k));
        orderImbalance->setTime(l);
        orderImbalance->setSequence(sequence);
        orderImbalance->setRefPrice(d);
        orderImbalance->setPairedSize(d);
        orderImbalance->setImbalanceSize(d);
        orderImbalance->setNearPrice(d);
        orderImbalance->setFarPrice(d);
        orderImbalance->setImbalanceSide(pick(imbalanceSides, k));
        orderImbalance->setAuctionType(pick(auctionTypes, k));
        events.push_back(orderImbalance);

        auto nuamOrder = std::make_shared<NuamOrder>(symbol);

        nuamOrder->setIndex(index);
        nuamOrder->setSource(pick(nuamSources, k));
        nuamOrder->setTime(l);
        nuamOrder->setSequence(sequence);
        nuamOrder->setPrice(d);
        nuamOrder->setSize(d);
        nuamOrder->setExchangeCode(c);
        nuamOrder->setOrderSide(pick(orderSides, k));
        nuamOrder->setScope(pick(scopes, k));
        nuamOrder->setActorId(i);
        nuamOrder->setParticipantId(i);
        nuamOrder->setSubmitterId(i);
        nuamOrder->setOnBehalfOfSubmitterId(i);
        nuamOrder->setClientOrderId(s);
        nuamOrder->setCustomerAccount(s);
        nuamOrder->setCustomerInfo(s);
        nuamOrder->setExchangeInfo(s);
        nuamOrder->setTimeInForce(NuamTimeInForceType::ALL[k % NuamTimeInForceType::ALL.size()]);
        nuamOrder->setTimeInForceData(i);
        nuamOrder->setTriggerOrderBookId(i);
        nuamOrder->setTriggerPrice(d);
        nuamOrder->setTriggerSessionType(i);
        nuamOrder->setOrderType(NuamOrderType::ALL[k % NuamOrderType::ALL.size()]);
        nuamOrder->setOrderQuantity(d);
        nuamOrder->setDisplayQuantity(d);
        nuamOrder->setRefreshQuantity(d);
        nuamOrder->setLeavesQuantity(d);
        nuamOrder->setMatchedQuantity(d);
        events.push_back(nuamOrder);

        auto nuamTrade = std::make_shared<NuamTrade>(symbol);

        nuamTrade->setTime(l);
        nuamTrade->setSequence(sequence);
        nuamTrade->setExchangeCode(c);
        nuamTrade->setPrice(d);
        nuamTrade->setSize(d);
        nuamTrade->setTickDirection(pick(directions, k));
        nuamTrade->setChange(d);
        nuamTrade->setDayId(i);
        nuamTrade->setDayVolume(d);
        nuamTrade->setDayTurnover(d);
        nuamTrade->setExtendedTradingHours(b);
        nuamTrade->setTradeStatTime(l);
        nuamTrade->setLastSignificantPrice(d);
        nuamTrade->setLastPriceForAll(d);
        nuamTrade->setNumberOfTrades(i);
        nuamTrade->setVWAP(d);
        events.push_back(nuamTrade);

        auto nuamTimeAndSale = std::make_shared<NuamTimeAndSale>(symbol);

        nuamTimeAndSale->setTime(l);
        nuamTimeAndSale->setSequence(sequence);
        nuamTimeAndSale->setExchangeCode(c);
        nuamTimeAndSale->setPrice(d);
        nuamTimeAndSale->setSize(d);
        nuamTimeAndSale->setBidPrice(d);
        nuamTimeAndSale->setAskPrice(d);
        nuamTimeAndSale->setExchangeSaleConditions(shortString);
        nuamTimeAndSale->setTradeThroughExempt(c);
        nuamTimeAndSale->setAggressorSide(pick(sides, k));
        nuamTimeAndSale->setSpreadLeg(b);
        nuamTimeAndSale->setExtendedTradingHours(!b);
        nuamTimeAndSale->setValidTick(b);
        nuamTimeAndSale->setType(pick(timeAndSaleTypes, k));
        nuamTimeAndSale->setMatchId(l);
        events.push_back(nuamTimeAndSale);

        // The market maker id keeps the last 4 characters of the short string, as in the Java API.
        auto marketMaker = std::make_shared<MarketMaker>(symbol);

        marketMaker->setExchangeCode(c);
        marketMaker->setMarketMaker(shortString);
        marketMaker->setBidTime(l);
        marketMaker->setBidPrice(d);
        marketMaker->setBidSize(d);
        marketMaker->setBidCount(l);
        marketMaker->setAskTime(l);
        marketMaker->setAskPrice(d);
        marketMaker->setAskSize(d);
        marketMaker->setAskCount(l);
        events.push_back(marketMaker);

        auto textConfiguration = std::make_shared<TextConfiguration>(symbol, s, i);

        textConfiguration->setTime(l);
        textConfiguration->setSequence(sequence);
        events.push_back(textConfiguration);
    }

    // All 256 values of the event flags byte on an Order with index 0 (setIndex() after setSource() resets the source
    // to DEFAULT, as in the Java API).
    for (std::int32_t flags = 0; flags < 256; flags++) {
        char symbol[32]{};

        std::snprintf(symbol, sizeof(symbol), "EDGE-FLAGS-%03d", flags);

        auto order = std::make_shared<Order>(symbol);

        order->setSource(OrderSource::NTV);
        order->setIndex(0);
        order->setOrderSide(Side::BUY);
        order->setPrice(1);
        order->setSize(1);
        order->setEventFlags(flags);
        events.push_back(order);
    }

    for (const auto &event : events) {
        event->setEventTime(T);
    }

    return events;
}

} // namespace dxfcpp::test
