// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

namespace dxfcpp::tools {

struct ConnectTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;

    struct Args {
        std::string address;
        std::string types;
        std::string symbols;
        std::optional<std::string> fromTime;
        std::optional<std::string> source;
        std::optional<std::string> properties;
        std::optional<std::string> tape;
        bool isQuite;

        static Args parse(const std::vector<std::string> &args) noexcept {
            return {};
        }
    };

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;

        auto endpoint =
            DXEndpoint::newBuilder()
                ->withRole(DXEndpoint::Role::FEED)
                ->withProperties(CmdArgsUtils::parseProperties(args.properties))
                ->withName(NAME + "Tool")
                ->build();

        std::shared_ptr<DXFeedSubscription> sub = endpoint->getFeed()->createSubscription(
            CmdArgsUtils::parseTypes(args.types));

        if (!args.isQuite) {
            sub->addEventListener([](auto &&events) {
                for (auto &&e : events) {
                    std::cout << e << "\n";
                }

                std::cout.flush();
            });
        }

        auto symbols = CmdArgsUtils::parseSymbols(args.symbols);

        if (args.fromTime.has_value()) {
            auto fromTime = CmdArgsUtils::parseDateTime(args.fromTime.value());

            symbols = symbols | ranges::views::transform([fromTime](const auto &sw) {
                          return TimeSeriesSubscriptionSymbol{sw, fromTime};
                      }) |
                      ranges::to<std::unordered_set<SymbolWrapper>>;
        } else if (args.source.has_value()) {
            auto source = OrderSource::valueOf(args.source.value());

            symbols = symbols | ranges::views::transform([source](const auto &sw) {
                          return IndexedEventSubscriptionSymbol{sw, source};
                      }) |
                      ranges::to<std::unordered_set<SymbolWrapper>>;
        }

        if (args.tape.has_value()) {
            std::string tape = args.tape.value();

            auto pub = DXEndpoint::newBuilder()
                           ->withRole(DXEndpoint::Role::STREAM_PUBLISHER)
                           ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default
                           ->withName(NAME + "Tool")
                           ->build()
                           ->connect(tape.starts_with("tape:") ? tape : "tape:" + tape)
                           ->getPublisher();

            sub->addEventListener([pub](auto &&events) {
                pub->publishEvents(events);
            });
        }

        sub->addSymbols(symbols);
        endpoint->connect(args.address);
        std::this_thread::sleep_for(std::chrono::days(365));
    }
};

} // namespace dxfcpp::tools