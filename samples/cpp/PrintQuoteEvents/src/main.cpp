// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

int main() {
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        using namespace dxfcpp::literals;

        std::vector<dxfcpp::SymbolWrapper> symbols{{"123", "123123"sv, "123213123"s, "*"_wcs, "ZZZ"_s}};

        for (const auto& s : symbols) {


            std::cerr << dxfcpp::graalSymbolToString(s.toGraal()) + "\n";
        }

        auto builder = dxfcpp::DXEndpoint::newBuilder()
                           ->withRole(dxfcpp::DXEndpoint::Role::FEED)
                           ->withProperty(dxfcpp::DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true");
        auto endpoint = builder->build();

        auto sub = endpoint->getFeed()->createSubscription({dxfcpp::EventTypeEnum::QUOTE});

        sub->addEventListener([](auto &&events) {
            for (const auto &e : events) {
                std::cout << e << std::endl;
            }
        });

        sub->addSymbols({"AAPL", "IBM"sv, "XBT/USD:GDAX"s, "BTC/EUR:CXBITF"_s, "*"_wcs});
        sub->addSymbols("AAPL");
        sub->addSymbols("IBM"sv);
        sub->addSymbols("TSLA"s);
        sub->addSymbols(dxfcpp::WildcardSymbol::ALL);
        sub->addSymbols("AMD"_s);
        sub->addSymbols("*"_wcs);
        sub->addSymbols(symbols[4]);
        sub->addSymbols("!@#"_sw);
        sub->addSymbols(std::vector{"11111"s});
        sub->addSymbols(std::vector{"AAPL", "IBM", "TSLA", "GOOG"});
        auto v = std::vector{"AAPL", "IBM"};
        auto v2 = std::vector<dxfcpp::SymbolWrapper>{"XBT/USD:GDAX"s, "BTC/EUR:CXBITF"sv, "TSLA", "GOOG"_s};

        sub->addSymbols(v);
        sub->addSymbols(v2);


//        endpoint->connect("demo.dxfeed.com:7300");
        endpoint->connect("mddqa.in.devexperts.com:7400");

        std::this_thread::sleep_for(std::chrono::seconds(5));

        sub->removeSymbols("TSLA");

        sub->removeSymbols(symbols);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        endpoint->close();
    }
}
