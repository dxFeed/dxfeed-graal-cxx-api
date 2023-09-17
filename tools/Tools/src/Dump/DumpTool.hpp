// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

namespace dxfcpp::tools {

struct DumpTool {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;

    struct Args {
        std::string address;
        std::optional<std::string> types;
        std::optional<std::string> symbols;
        std::optional<std::string> properties;
        std::optional<std::string> tape;
        bool isQuite;

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            auto r = AddressArgRequired::parse(args);

            if (r.isError) {
                return ParseResult<Args>::error(r.errorString);
            }

            return ParseResult<Args>::ok({r.result, {}, {}, {}, {}, false});
        }
    };

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;

        auto properties = CmdArgsUtils::parseProperties(args.properties);

        auto inputEndpoint =
            DXEndpoint::newBuilder()
                ->withRole(DXEndpoint::Role::STREAM_FEED)
                ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default
                ->withProperties(properties)
                ->withName(NAME + "Tool")
                ->build();

        auto sub = inputEndpoint->getFeed()->createSubscription(
            !args.types.has_value() ? CmdArgsUtils::parseTypes("all") : CmdArgsUtils::parseTypes(args.types.value()));

        if (!args.isQuite) {
            sub->addEventListener([](auto &&events) {
                for (auto &&e : events) {
                    std::cout << e << "\n";
                }

                std::cout.flush();
            });
        }

        std::optional<DXEndpoint::Ptr> outputEndpoint{};

        if (args.tape.has_value()) {
            std::string tape = args.tape.value();

            outputEndpoint =
                DXEndpoint::newBuilder()
                    ->withRole(DXEndpoint::Role::STREAM_PUBLISHER)
                    ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default
                    ->withProperties(properties)
                    ->withName(NAME + "Tool")
                    ->build()
                    ->connect(tape.starts_with("tape:") ? tape : "tape:" + tape);

            sub->addEventListener([endpoint = outputEndpoint.value()](auto &&events) {
                endpoint->getPublisher()->publishEvents(events);
            });
        }

        sub->addSymbols(!args.symbols.has_value() ? CmdArgsUtils::parseSymbols("all")
                                                  : CmdArgsUtils::parseSymbols(args.symbols.value()));

        inputEndpoint->connect(args.address);
        inputEndpoint->awaitNotConnected();
        inputEndpoint->closeAndAwaitTermination();

        if (outputEndpoint.has_value()) {
            outputEndpoint.value()->awaitProcessed();
            outputEndpoint.value()->closeAndAwaitTermination();
        }
    }
};

} // namespace dxfcpp::tools