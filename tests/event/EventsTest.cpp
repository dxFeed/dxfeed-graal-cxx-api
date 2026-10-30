// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cmath>
#include <string>

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

TEST_CASE("An exchange code above ASCII is kept as a Latin-1 character and printed as Java prints it") {
    Quote quote{"AAPL"};
    Trade trade{"AAPL"};
    TimeAndSale timeAndSale{"AAPL"};
    OptionSale optionSale{"AAPL"};
    MarketMaker marketMaker{"AAPL"};

    quote.setBidExchangeCode('\xE2');
    quote.setAskExchangeCode('\xFF');
    trade.setExchangeCode('\xC4');
    timeAndSale.setExchangeCode('\xC4');
    optionSale.setExchangeCode('\x80');
    marketMaker.setExchangeCode('\xE2');

    CHECK(quote.getBidExchangeCode() == 0xE2);
    CHECK(quote.getAskExchangeCode() == 0xFF);
    CHECK(trade.getExchangeCode() == 0xC4);
    CHECK(timeAndSale.getExchangeCode() == 0xC4);
    CHECK(optionSale.getExchangeCode() == 0x80);
    CHECK(marketMaker.getExchangeCode() == 0xE2);
    CHECK(quote.toString().find("bidExchange=\\u00e2, ") != std::string::npos);
    CHECK(quote.toString().find("askExchange=\\u00ff, ") != std::string::npos);
}

TEST_CASE("The Candle::assign method must set all fields") {
    auto c1 = std::make_shared<Candle>(CandleSymbol::valueOf("AAPL&Q"));
    c1->setEventTime(100);

    auto c2 = std::make_shared<Candle>(CandleSymbol::valueOf("IBM"));

    c2->setEventTime(1200);

    // index
    c2->setTime(222);
    c2->setSequence(23);

    c2->setCount(5);

    c2->setOpen(50.1);
    c2->setHigh(100.2);
    c2->setLow(40.3);
    c2->setClose(34.4);

    c2->setVolume(1000.34);
    c2->setVWAP(34.22);
    c2->setBidVolume(1234.33);
    c2->setAskVolume(14.44);
    c2->setImpVolatility(200);
    c2->setOpenInterest(200);

    auto c3 = std::make_shared<Candle>();

    c3->assign(c1);
    c1->assign(c2);

    REQUIRE(c1->toString() == c2->toString());
    REQUIRE(c1->toString() != c3->toString());
}

TEST_CASE("The day volume and the day turnover of a new trade are NaN, as in Java") {
    const Trade trade{};
    const TradeETH tradeEth{};
    const NuamTrade nuamTrade{};

    CHECK(std::isnan(trade.getDayVolume()));
    CHECK(std::isnan(trade.getDayTurnover()));
    CHECK(std::isnan(tradeEth.getDayVolume()));
    CHECK(std::isnan(tradeEth.getDayTurnover()));
    CHECK(std::isnan(nuamTrade.getDayVolume()));
    CHECK(std::isnan(nuamTrade.getDayTurnover()));
}

TEST_CASE("The text of a new TextMessage is null, as in Java") {
    const TextMessage message("AAPL");

    CHECK(!message.getTextOpt());
    CHECK(message.getText() == String::NUL);
    CHECK(TextMessage("AAPL", "").getTextOpt() == std::optional<std::string>{""});
}

TEST_CASE("The trade id of a new trade is 0 and is kept by the setter and assign()") {
    auto trade = std::make_shared<Trade>("AAPL");
    auto timeAndSale = std::make_shared<TimeAndSale>("AAPL");

    CHECK(trade->getTradeId() == 0);
    CHECK(timeAndSale->getTradeId() == 0);

    trade->setTradeId(-1);
    timeAndSale->setTradeId(9'876'543'210);

    auto tradeCopy = std::make_shared<Trade>();
    auto timeAndSaleCopy = std::make_shared<TimeAndSale>();

    tradeCopy->assign(trade);
    timeAndSaleCopy->assign(timeAndSale);

    CHECK(tradeCopy->getTradeId() == -1);
    CHECK(timeAndSaleCopy->getTradeId() == 9'876'543'210);
}
