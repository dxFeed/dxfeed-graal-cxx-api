// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "EventFactory.hpp"

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
    };

    for (const auto &event : events) {
        event->setEventTime(T);
    }

    return events;
}

} // namespace dxfcpp::test
