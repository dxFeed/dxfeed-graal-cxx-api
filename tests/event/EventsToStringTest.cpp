// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <limits>
#include <string>

using namespace dxfcpp;

// The expected strings are the output of Java `toString()` of the same events (dxFeed Java API 3.356). Change them only
// from a new run of the Java probe, not by hand. The times are zero, so the time zone does not matter.

TEST_CASE("Quote::toString() is the same as in Java") {
    Quote quote("AAPL");

    quote.setBidPrice(1.0E-5);
    quote.setBidSize(9.0E-4);
    quote.setAskPrice(2.5E8);
    quote.setAskSize(1.0E21);

    CHECK(quote.toString() ==
          "Quote{AAPL, eventTime=0, time=0, timeNanoPart=0, sequence=0, bidTime=0, bidExchange=\\0, bidPrice=1.0E-5, "
          "bidSize=9.0E-4, askTime=0, askExchange=\\0, askPrice=2.5E8, askSize=1.0E21}");
}

TEST_CASE("Trade::toString() and TradeETH::toString() are the same as in Java") {
    Trade trade("AAPL");

    trade.setPrice(150.25);
    trade.setChange(-9.0E-4);
    trade.setSize(1.0E-5);
    trade.setDayVolume(12345678.0);
    trade.setDayTurnover(2.5E8);

    CHECK(trade.toString() ==
          "Trade{AAPL, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, price=150.25, "
          "change=-9.0E-4, size=1.0E-5, day=19700101, dayVolume=1.2345678E7, dayTurnover=2.5E8, direction=UNDEFINED, "
          "ETH=false}");

    TradeETH tradeEth("AAPL");

    tradeEth.setPrice(1.0E7);
    tradeEth.setDayVolume(0.001);
    tradeEth.setDayTurnover(1.0E21);

    CHECK(tradeEth.toString() ==
          "TradeETH{AAPL, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, price=1.0E7, "
          "change=NaN, size=NaN, day=19700101, dayVolume=0.001, dayTurnover=1.0E21, direction=UNDEFINED, ETH=false}");
}

TEST_CASE("Default Trade, TradeETH and TimeAndSale print as in Java") {
    CHECK(Trade("AAPL").toString() ==
          "Trade{AAPL, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, price=NaN, "
          "change=NaN, size=NaN, day=19700101, dayVolume=NaN, dayTurnover=NaN, direction=UNDEFINED, ETH=false}");
    CHECK(TradeETH("AAPL").toString() ==
          "TradeETH{AAPL, eventTime=0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, tradeId=0, price=NaN, "
          "change=NaN, size=NaN, day=19700101, dayVolume=NaN, dayTurnover=NaN, direction=UNDEFINED, ETH=false}");
    CHECK(TimeAndSale("AAPL").toString() ==
          "TimeAndSale{AAPL, eventTime=0, eventFlags=0x0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, "
          "tradeId=0, price=NaN, size=NaN, bid=NaN, ask=NaN, ESC='null', TTE=\\0, side=UNDEFINED, spread=false, "
          "ETH=false, validTick=false, type=NEW}");
}

TEST_CASE("The trade id is printed as a signed decimal, as in Java") {
    Trade trade("AAPL");
    TimeAndSale timeAndSale("AAPL");

    trade.setTradeId(12345);
    timeAndSale.setTradeId(-1);

    CHECK(trade.toString().find(", exchange=\\0, tradeId=12345, price=NaN, ") != std::string::npos);
    CHECK(timeAndSale.toString().find(", exchange=\\0, tradeId=-1, price=NaN, ") != std::string::npos);

    trade.setTradeId(std::numeric_limits<std::int64_t>::min());

    CHECK(trade.toString().find(", tradeId=-9223372036854775808, ") != std::string::npos);
}

TEST_CASE("TextMessage::toString() prints a null text as in Java") {
    CHECK(TextMessage("AAPL").toString() == "TextMessage{AAPL, eventTime=0, time=0, sequence=0, text='null'}");
}

TEST_CASE("TimeAndSale::toString() is the same as in Java") {
    TimeAndSale timeAndSale("AAPL");

    timeAndSale.setPrice(2.5E8);
    timeAndSale.setSize(1.0E-5);
    timeAndSale.setBidPrice(9.0E-4);
    timeAndSale.setAskPrice(1.0E21);

    CHECK(timeAndSale.toString() ==
          "TimeAndSale{AAPL, eventTime=0, eventFlags=0x0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, "
          "tradeId=0, price=2.5E8, size=1.0E-5, bid=9.0E-4, ask=1.0E21, ESC='null', TTE=\\0, side=UNDEFINED, "
          "spread=false, ETH=false, validTick=false, type=NEW}");
}

TEST_CASE("OptionSale::toString() is the same as in Java") {
    OptionSale sale(".AAPL");

    sale.setPrice(1.0E7);
    sale.setSize(1.0E-5);
    sale.setUnderlyingPrice(2.5E8);
    sale.setVolatility(9.0E-4);
    sale.setDelta(1.0E21);

    CHECK(sale.toString() ==
          "OptionSale{.AAPL, eventTime=0, eventFlags=0x0, index=0x0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, "
          "price=1.0E7, size=1.0E-5, bid=NaN, ask=NaN, ESC='null', TTE=\\0, side=UNDEFINED, spread=false, ETH=false, "
          "validTick=false, type=NEW, underlyingPrice=2.5E8, volatility=9.0E-4, delta=1.0E21, optionSymbol='null'}");

    OptionSale strings(".AAPL");

    strings.setExchangeSaleConditions("");
    strings.setOptionSymbol(".AAPL240119C150");

    CHECK(strings.toString() ==
          "OptionSale{.AAPL, eventTime=0, eventFlags=0x0, index=0x0, time=0, timeNanoPart=0, sequence=0, exchange=\\0, "
          "price=NaN, size=NaN, bid=NaN, ask=NaN, ESC='', TTE=\\0, side=UNDEFINED, spread=false, ETH=false, "
          "validTick=false, type=NEW, underlyingPrice=NaN, volatility=NaN, delta=NaN, optionSymbol='.AAPL240119C150'}");
}

TEST_CASE("Profile::toString() is the same as in Java") {
    CHECK(Profile("AAPL").toString() ==
          "Profile{AAPL, eventTime=0, description='null', SSR=UNDEFINED, status=UNDEFINED, statusReason='null', "
          "haltStartTime=0, haltEndTime=0, highLimitPrice=NaN, lowLimitPrice=NaN, high52WeekPrice=NaN, "
          "low52WeekPrice=NaN, beta=NaN, earningsPerShare=NaN, dividendFrequency=NaN, exDividendAmount=NaN, "
          "exDividendDay=19700101, shares=NaN, freeFloat=NaN}");

    Profile profile("AAPL");

    profile.setDescription("");
    profile.setStatusReason("halted");
    profile.setHighLimitPrice(1.0E7);
    profile.setBeta(9.0E-4);
    profile.setEarningsPerShare(1.0E-5);

    CHECK(profile.toString() ==
          "Profile{AAPL, eventTime=0, description='', SSR=UNDEFINED, status=UNDEFINED, statusReason='halted', "
          "haltStartTime=0, haltEndTime=0, highLimitPrice=1.0E7, lowLimitPrice=NaN, high52WeekPrice=NaN, "
          "low52WeekPrice=NaN, beta=9.0E-4, earningsPerShare=1.0E-5, dividendFrequency=NaN, exDividendAmount=NaN, "
          "exDividendDay=19700101, shares=NaN, freeFloat=NaN}");

    Profile shares("AAPL");

    shares.setShares(1.0E7);
    shares.setFreeFloat(9.0E-4);

    CHECK(shares.toString() ==
          "Profile{AAPL, eventTime=0, description='null', SSR=UNDEFINED, status=UNDEFINED, statusReason='null', "
          "haltStartTime=0, haltEndTime=0, highLimitPrice=NaN, lowLimitPrice=NaN, high52WeekPrice=NaN, "
          "low52WeekPrice=NaN, beta=NaN, earningsPerShare=NaN, dividendFrequency=NaN, exDividendAmount=NaN, "
          "exDividendDay=19700101, shares=1E7, freeFloat=0.0009}");
}

TEST_CASE("Profile::toString() prints shares and freeFloat as the Java WideDecimal") {
    const auto sharesOf = [](double shares) {
        Profile profile("AAPL");

        profile.setShares(shares);

        const auto s = profile.toString();
        const auto begin = s.find("shares=") + 7;

        return s.substr(begin, s.find(", freeFloat=") - begin);
    };

    CHECK(sharesOf(1000.0) == "1000");
    CHECK(sharesOf(1.0E7) == "1E7");
    CHECK(sharesOf(1.0E6) == "1000000");
    CHECK(sharesOf(1.0E9) == "1E9");
    CHECK(sharesOf(12345678901234.0) == "12345678901234");
    CHECK(sharesOf(1.23E15) == "1.23E15");
    CHECK(sharesOf(9.0E-4) == "0.0009");
    CHECK(sharesOf(1.0E-7) == "0.0000001");
    CHECK(sharesOf(1.0E-12) == "1E-12");
    CHECK(sharesOf(1.0 / 3.0) == "0.333333333333333");
    CHECK(sharesOf(1234567.8912345679) == "1234567.891234568");
    CHECK(sharesOf(-1000.0) == "-1000");
    CHECK(sharesOf(-9.0E-4) == "-0.0009");
    CHECK(sharesOf(-1.5) == "-1.5");
    CHECK(sharesOf(-1.0E-12) == "-1E-12");
    CHECK(sharesOf(-0.333333333333333) == "-0.333333333333333");
    CHECK(sharesOf(9.007199254740992E15) == "9007199254740992");
    CHECK(sharesOf(3.602879701896397E16) == "36028797018963970");
    CHECK(sharesOf(1.2345678901234568E17) == "123456789012345680");
    CHECK(sharesOf(1.0E17) == "1E17");
    CHECK(sharesOf(1.0E18) == "1E18");
    CHECK(sharesOf(9.3E18) == "9.3E18");
    CHECK(sharesOf(9.223372036854776E18) == "9223372036854776000");
    CHECK(sharesOf(1.2345678901234567E19) == "12345678901234570000");
    CHECK(sharesOf(1.0E20) == "1E20");
    CHECK(sharesOf(-1.0E20) == "-1E20");
    CHECK(sharesOf(1.0E100) == "1E100");
    CHECK(sharesOf(-1.7E308) == "-Infinity");
    CHECK(sharesOf(0.0) == "0");
    CHECK(sharesOf(std::numeric_limits<double>::quiet_NaN()) == "NaN");
    CHECK(sharesOf(std::numeric_limits<double>::infinity()) == "Infinity");
    CHECK(sharesOf(-std::numeric_limits<double>::infinity()) == "-Infinity");
    CHECK(sharesOf(4.9E-324) == "0");
    CHECK(sharesOf(1.7E308) == "Infinity");
}

TEST_CASE("A negative index is printed as an unsigned hexadecimal number, as Java Long.toHexString prints it") {
    const auto contains = [](const std::string &s, const std::string &part) {
        return s.find(part) != std::string::npos;
    };

    OptionSale sale(".AAPL");
    Series series("AAPL");
    Order order("AAPL");

    sale.setIndex(-1);
    series.setIndex(-1);
    order.setIndex(0x4e545612345678);

    CHECK(contains(sale.toString(), ", index=0xffffffffffffffff, "));
    CHECK(contains(series.toString(), ", index=0xffffffffffffffff, "));
    CHECK(contains(order.toString(), ", index=0x4e545612345678, "));
}

TEST_CASE("TimeAndSale::toString() prints the buyer and the seller after the type, as Java, only if they are set") {
    TimeAndSale emptyBuyer("AAPL");
    TimeAndSale both("AAPL");

    emptyBuyer.setBuyer("");
    both.setBuyer("B1");
    both.setSeller("S'1");

    CHECK(emptyBuyer.toString().ends_with(", type=NEW, buyer=''}"));
    CHECK(both.toString().ends_with(", type=NEW, buyer='B1', seller='S'1'}"));
    CHECK(TimeAndSale("AAPL").toString().ends_with(", type=NEW}"));
}

TEST_CASE("Order::toString() is the same as in Java") {
    CHECK(Order("AAPL").toString() ==
          "Order{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, timeNanoPart=0, "
          "action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, executedSize=NaN, count=0, "
          "exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, tradeSize=NaN, "
          "marketMaker='null'}");

    Order order("AAPL");

    order.setPrice(1.0E7);
    order.setSize(1.0E-5);
    order.setExecutedSize(2.5E8);
    order.setTradePrice(9.0E-4);
    order.setTradeSize(1.0E21);
    order.setMarketMaker("NSDQ");

    CHECK(order.toString() ==
          "Order{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, timeNanoPart=0, "
          "action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=1.0E7, size=1.0E-5, executedSize=2.5E8, "
          "count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=9.0E-4, tradeSize=1.0E21, "
          "marketMaker='NSDQ'}");

    Order empty("AAPL");

    empty.setMarketMaker("");

    CHECK(empty.toString() ==
          "Order{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, timeNanoPart=0, "
          "action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, executedSize=NaN, count=0, "
          "exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, tradeSize=NaN, marketMaker=''}");
}

TEST_CASE("SpreadOrder::toString() is the same as in Java") {
    CHECK(SpreadOrder("AAPL").toString() ==
          "SpreadOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, spreadSymbol='null'}");

    SpreadOrder spread("AAPL");

    spread.setSpreadSymbol("AAPL-MSFT");

    CHECK(spread.toString() ==
          "SpreadOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, spreadSymbol='AAPL-MSFT'}");
}

TEST_CASE("AnalyticOrder::toString() is the same as in Java") {
    CHECK(AnalyticOrder("AAPL").toString() ==
          "AnalyticOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, marketMaker='null', icebergPeakSize=NaN, icebergHiddenSize=NaN, icebergExecutedSize=NaN, "
          "icebergType=UNDEFINED}");

    AnalyticOrder order("AAPL");

    order.setMarketMaker("NSDQ");
    order.setIcebergPeakSize(1.0E7);
    order.setIcebergHiddenSize(1.0E-5);
    order.setIcebergExecutedSize(2.5E8);

    CHECK(order.toString() ==
          "AnalyticOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, marketMaker='NSDQ', icebergPeakSize=1.0E7, icebergHiddenSize=1.0E-5, "
          "icebergExecutedSize=2.5E8, icebergType=UNDEFINED}");
}

TEST_CASE("OtcMarketsOrder::toString() is the same as in Java") {
    CHECK(OtcMarketsOrder("AAPL").toString() ==
          "OtcMarketsOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=NaN, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, marketMaker='null', QAP=0, open=false, unsolicited=false, priceType=UNPRICED, "
          "saturated=false, autoEx=false, NMS=false}");

    OtcMarketsOrder order("AAPL");

    order.setMarketMaker("NSDQ");
    order.setPrice(9.0E-4);

    CHECK(order.toString() ==
          "OtcMarketsOrder{AAPL, eventTime=0, source=DEFAULT, eventFlags=0x0, index=0x0, time=0, sequence=0, "
          "timeNanoPart=0, action=UNDEFINED, actionTime=0, orderId=0, auxOrderId=0, price=9.0E-4, size=NaN, "
          "executedSize=NaN, count=0, exchange=\\0, side=UNDEFINED, scope=COMPOSITE, tradeId=0, tradePrice=NaN, "
          "tradeSize=NaN, marketMaker='NSDQ', QAP=0, open=false, unsolicited=false, priceType=UNPRICED, "
          "saturated=false, autoEx=false, NMS=false}");
}

TEST_CASE("Summary::toString() is the same as in Java") {
    Summary summary("AAPL");

    summary.setDayOpenPrice(1.0E7);
    summary.setDayHighPrice(9.0E-4);
    summary.setDayLowPrice(2.5E8);
    summary.setDayClosePrice(1.0E21);
    summary.setPrevDayClosePrice(1.0E-5);
    summary.setPrevDayVolume(12345678.0);

    CHECK(summary.toString() ==
          "Summary{AAPL, eventTime=0, day=19700101, dayOpen=1.0E7, dayHigh=9.0E-4, dayLow=2.5E8, dayClose=1.0E21, "
          "dayCloseType=REGULAR, prevDay=19700101, prevDayClose=1.0E-5, prevDayCloseType=REGULAR, "
          "prevDayVolume=1.2345678E7, openInterest=0}");
}

TEST_CASE("Greeks, Series, TheoPrice and Underlying toString() are the same as in Java") {
    Greeks greeks(".AAPL");

    greeks.setPrice(1.0E7);
    greeks.setVolatility(9.0E-4);
    greeks.setDelta(1.0E-5);
    greeks.setGamma(2.5E8);
    greeks.setTheta(1.0E21);
    greeks.setRho(-1.0E-5);
    greeks.setVega(0.001);

    CHECK(greeks.toString() == "Greeks{.AAPL, eventTime=0, eventFlags=0x0, time=0, sequence=0, price=1.0E7, "
                               "volatility=9.0E-4, delta=1.0E-5, gamma=2.5E8, theta=1.0E21, rho=-1.0E-5, vega=0.001}");

    Series series("AAPL");

    series.setVolatility(9.0E-4);
    series.setCallVolume(1.0E7);
    series.setPutVolume(2.5E8);
    series.setPutCallRatio(1.0E-5);
    series.setForwardPrice(1.0E21);
    series.setDividend(0.001);
    series.setInterest(9999999.0);

    CHECK(series.toString() ==
          "Series{AAPL, eventTime=0, eventFlags=0x0, index=0x0, time=0, sequence=0, expiration=19700101, "
          "volatility=9.0E-4, callVolume=1.0E7, putVolume=2.5E8, putCallRatio=1.0E-5, forwardPrice=1.0E21, "
          "dividend=0.001, interest=9999999.0}");

    TheoPrice theoPrice(".AAPL");

    theoPrice.setPrice(1.0E7);
    theoPrice.setUnderlyingPrice(2.5E8);
    theoPrice.setDelta(9.0E-4);
    theoPrice.setGamma(1.0E-5);
    theoPrice.setDividend(1.0E21);
    theoPrice.setInterest(-0.0);

    CHECK(theoPrice.toString() == "TheoPrice{.AAPL, eventTime=0, eventFlags=0x0, time=0, sequence=0, price=1.0E7, "
                                  "underlyingPrice=2.5E8, delta=9.0E-4, gamma=1.0E-5, dividend=1.0E21, interest=-0.0}");

    Underlying underlying("AAPL");

    underlying.setVolatility(9.0E-4);
    underlying.setFrontVolatility(1.0E-5);
    underlying.setBackVolatility(1.0E7);
    underlying.setCallVolume(2.5E8);
    underlying.setPutVolume(1.0E21);
    underlying.setPutCallRatio(std::numeric_limits<double>::infinity());

    CHECK(underlying.toString() ==
          "Underlying{AAPL, eventTime=0, eventFlags=0x0, time=0, sequence=0, volatility=9.0E-4, "
          "frontVolatility=1.0E-5, backVolatility=1.0E7, callVolume=2.5E8, putVolume=1.0E21, putCallRatio=Infinity}");
}

TEST_CASE("Candle::toString() is the same as in Java") {
    Candle candle(CandleSymbol::valueOf("AAPL{=d}"));

    candle.setOpen(1.0E7);
    candle.setHigh(2.5E8);
    candle.setLow(9.0E-4);
    candle.setClose(1.0E-5);
    candle.setVolume(1.0E21);
    candle.setVWAP(0.001);
    candle.setBidVolume(12345678.0);
    candle.setAskVolume(-std::numeric_limits<double>::infinity());
    candle.setImpVolatility(1.0E-4);
    candle.setOpenInterest(1.0E16);

    CHECK(candle.toString() ==
          "Candle{AAPL{=d}, eventTime=0, eventFlags=0x0, time=0, sequence=0, count=0, open=1.0E7, high=2.5E8, "
          "low=9.0E-4, close=1.0E-5, volume=1.0E21, vwap=0.001, bidVolume=1.2345678E7, askVolume=-Infinity, "
          "impVolatility=1.0E-4, openInterest=1.0E16}");
}
