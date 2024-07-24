// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

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
        std::optional<std::string> types;
        std::optional<std::string> symbols;
        std::optional<std::string> properties;
        std::optional<std::string> tape;
        bool isQuite;

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            auto parsedAddress = AddressArgRequired::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            index++;

            std::optional<std::string> types;
            std::optional<std::string> symbols;

            if (args.size() > index && args[index].starts_with('-')) {
                types = std::nullopt;
                symbols = std::nullopt;
            } else {
                auto parsedTypes = TypesArg::parse(args);

                if (parsedTypes.result.has_value()) {
                    index++;
                }

                auto parsedSymbols = SymbolsArg::parse(args);

                if (parsedSymbols.result.has_value()) {
                    index++;
                }

                types = parsedTypes.result;
                symbols = parsedSymbols.result;
            }

            bool propertiesIsParsed{};
            std::optional<std::string> properties{};
            bool tapeIsParsed{};
            std::optional<std::string> tape{};
            bool isQuite{};

            for (; index < args.size();) {
                if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
                    auto parseResult = PropertiesArg::parse(args, index);

                    properties = parseResult.result;
                    propertiesIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!tapeIsParsed && TapeArg::canParse(args, index)) {
                    auto parseResult = TapeArg::parse(args, index);

                    tape = parseResult.result;
                    tapeIsParsed = true;
                    index = parseResult.nextIndex;
                } else {
                    if (!isQuite && (isQuite = QuiteArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    index++;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, types, symbols, properties, tape, isQuite});
        }
    };

    static void run(const Args &args) noexcept {
        try {
            using namespace std::literals;

            auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);

            System::setProperties(parsedProperties);

            auto inputEndpoint =
                DXEndpoint::newBuilder()
                    ->withRole(DXEndpoint::Role::STREAM_FEED)
                    ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default
                    ->withProperties(parsedProperties)
                    ->withName(NAME + "Tool-Feed")
                    ->build();

            auto sub = inputEndpoint->getFeed()->createSubscription(
                !args.types.has_value() ? CmdArgsUtils::parseTypes("all") : CmdArgsUtils::parseTypes(*args.types));

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
                        ->withProperties(parsedProperties)
                        ->withName(NAME + "Tool-Publisher")
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
        } catch (const RuntimeException &e) {
            std::cerr << e << '\n';
        }
    }
};

} // namespace dxfcpp::tools