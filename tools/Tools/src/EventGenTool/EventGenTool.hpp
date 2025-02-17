// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <deque>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"
#include "dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "dxfeed_graal_cpp_api/api/DXFeed.hpp"
#include "dxfeed_graal_cpp_api/api/DXPublisher.hpp"
#include "dxfeed_graal_cpp_api/internal/Common.hpp"
#include "dxfeed_graal_cpp_api/internal/Timer.hpp"
#include "dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace dxfcpp::tools {

struct EventGenTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<ArgType> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t) noexcept {
        return fmt::format("{:{}}{:<{}}{:{}}{}\n", "", namePadding, getFullName(), nameFieldSize - 2 * namePadding, "",
                           namePadding, SHORT_DESCRIPTION);
    }

    struct Args {
        std::string address;
        std::optional<std::string> properties;

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            const auto parsedAddress = AddressArgRequired<>::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            index++;
            bool propertiesIsParsed{};
            std::optional<std::string> properties{};

            while (index < args.size()) {
                if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    const auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, properties});
        }
    };

    static void run(const Args &args) noexcept {
        try {
            using namespace std::literals;

            auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);

            System::setProperties(parsedProperties);

            const auto endpoint = DXEndpoint::newBuilder()
                                ->withRole(DXEndpoint::Role::PUBLISHER)
                                ->withProperties(parsedProperties)
                                ->withName(NAME + "Tool")
                                ->build();

            auto pub = endpoint->getPublisher();
            const auto sub = pub->getSubscription(Quote::TYPE);

            std::mutex symbolsMutex{};
            std::deque<SymbolWrapper> symbols{};

            sub->addChangeListener(
                ObservableSubscriptionChangeListener::create([&symbolsMutex, &symbols](const auto &symbolsToAdd) {
                    std::lock_guard lockGuard(symbolsMutex);

                    symbols.insert(symbols.end(), symbolsToAdd.begin(), symbolsToAdd.end());
                }));

            endpoint->connect(args.address);

            std::atomic<std::size_t> counter = 0;

            const auto scheduler = Timer::schedule(
                [&symbolsMutex, &symbols, &pub, &counter] {
                    std::lock_guard lockGuard(symbolsMutex);

                    if (symbols.empty()) {
                        return;
                    }

                    const auto& symbol = symbols.at(counter++ % symbols.size());
                    const auto q = std::make_shared<Quote>(symbol.asStringSymbol());

                    const auto time = now();
                    q->setEventTime(time);
                    q->setBidTime(time);
                    q->setBidPrice(42.0);
                    q->setBidSize(5.0);
                    q->setBidExchangeCode('@');
                    q->setAskTime(time);
                    q->setAskPrice(43.0);
                    q->setAskSize(5.0);
                    q->setAskExchangeCode('@');

                    pub->publishEvents(q);
                },
                30ms, 30ms);

            std::cin.get();

            scheduler->stop();
        } catch (const RuntimeException &e) {
            std::cerr << e << '\n';
        }
    }
};

} // namespace dxfcpp::tools