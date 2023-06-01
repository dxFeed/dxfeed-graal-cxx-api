// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <iostream>

int main() {
    using namespace dxfcpp;

    auto endpoint = DXEndpoint::newBuilder()->withProperty("dxfeed.address", "demo.dxfeed.com:7300")->build();

    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

    subscription->addEventListener([](auto &&events) {
        for (auto &&e : events) {
            std::cout << e << "\n";
        }
    });

    subscription->addSymbols({"AAPL"});

    std::cin.get();

    [] {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        using namespace dxfcpp::literals;

        std::vector<dxfcpp::EventTypeEnum> vi{};

        auto eventTypes = {dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE};

        dxfcpp::DXFeedSubscription::create(eventTypes.begin(), eventTypes.end());
        auto sub2 =
            dxfcpp::DXFeedSubscription::create(std::unordered_set{dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});

        std::vector<dxfcpp::SymbolWrapper> symbols{{"123", "123123"sv, "123213123"s, "*"_wcs, "ZZZ"_s}};

        for (const auto &s : symbols) {

            std::cerr << dxfcpp::graalSymbolToString(s.toGraal()) + "\n";
        }

        auto builder = dxfcpp::DXEndpoint::newBuilder()
                           ->withRole(dxfcpp::DXEndpoint::Role::FEED)
                           ->withProperty(dxfcpp::DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true");
        auto endpoint = builder->build();

        auto sub = endpoint->getFeed()->createSubscription({dxfcpp::Quote::TYPE, dxfcpp::TimeAndSale::TYPE});

        //        sub->addEventListener([](auto &&events) {
        //            for (const auto &e : events) {
        //                std::cout << e << std::endl;
        //            }
        //        });

        sub->addEventListener([](auto &&events) {
            for (const auto &e : events) {
                if (auto quote = e->template sharedAs<dxfcpp::Quote>(); quote) {
                    std::cout << "Q   : " + quote->toString() << std::endl;
                } else if (auto tns = e->template sharedAs<dxfcpp::TimeAndSale>(); tns) {
                    std::cout << "TnS : " + tns->toString() << std::endl;
                }
            }
        });

        auto id2 = sub->addEventListener(
            std::function([](const std::vector<std::shared_ptr<dxfcpp::TimeAndSale>> &timeAndSales) -> void {
                for (const auto &tns : timeAndSales) {
                    std::cout << "TnS2: " + tns->toString() << std::endl;
                }
            }));

        sub->addEventListener(std::function([](const std::vector<std::shared_ptr<dxfcpp::Trade>> &trades) -> void {
            for (const auto &trade : trades) {
                std::cout << "Trade: " + trade->toString() << std::endl;
            }
        }));

        sub->addEventListener<dxfcpp::TimeAndSale>([](const auto &timeAndSales) -> void {
            for (const auto &tns : timeAndSales) {
                std::cout << "TnS3: " + tns->toString() << std::endl;
            }
        });

        sub->addSymbols({"AAPL&Q", "IBM"sv, "XBT/USD:GDAX"s, "BTC/EUR:CXBITF"_s, "*"_wcs});
        sub->addSymbols("META");
        sub->addSymbols("ADDYY"sv);
        sub->addSymbols("MSFT"s);
        sub->addSymbols(dxfcpp::WildcardSymbol::ALL);
        sub->addSymbols("TCELL:TR"_s);
        sub->addSymbols("*"_wcs);
        sub->addSymbols("PFE"_sw);
        sub->addSymbols(std::vector{"CSCO"s});
        sub->addSymbols(std::vector{"$TOP10L/Q", "$SP500#45", "$TICK", "SPX"});

        auto v = std::vector{"$TOP10G/Q", "30Y:SME"};
        auto v2 = std::vector<dxfcpp::SymbolWrapper>{"$DECN"s, "./E1AN23P3580:XCME"sv, "/ESZ23:XCME", "/ESH25:XCME"_s};

        sub->addSymbols(v);
        sub->addSymbols(v2);
        sub->addSymbols(v2.begin(), v2.end());

        // D 230530 103822.766 [8212] DXFeedSubscription{000002672C607310}::addSymbols(symbols =
        // ['SymbolWrapper{StringSymbol{333}}', 'SymbolWrapper{StringSymbol{222}}', 'SymbolWrapper{StringSymbol{111}}'])
        auto us = std::unordered_set<dxfcpp::SymbolWrapper>{"111"s, "222"sv, "333", "111"_s};

        sub->addSymbols(us);

        // D 230530 103822.766 [8212] DXFeedSubscription{000002672C607310}::addSymbols(symbols =
        // ['SymbolWrapper{WildcardSymbol{*}}', 'SymbolWrapper{StringSymbol{*}}'])
        sub->addSymbols(std::unordered_set<dxfcpp::SymbolWrapper>{"*", "*"_wcs});

        //        endpoint->connect("demo.dxfeed.com:7300");
        endpoint->connect("mddqa.in.devexperts.com:7400");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        sub->removeSymbols("TSLA");

        sub->removeSymbols(symbols);

        sub->onEvent() -= id2;

        std::cerr << " ----------------------- \n";

        std::this_thread::sleep_for(std::chrono::seconds(5));

        endpoint->close();
    };
}
