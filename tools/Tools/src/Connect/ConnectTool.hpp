// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <iostream>
#include <memory>
#include <utility>
#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

namespace dxfcpp::tools {

struct ConnectTool {
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
        std::string types;
        std::string symbols;
        std::optional<std::string> fromTime;
        std::optional<std::string> source;
        std::optional<std::string> properties;
        std::optional<std::string> tape;
        bool isQuite{};
        bool forceStream{};

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            auto parsedAddress = AddressArgRequired<>::parse(args);

            if (parsedAddress.isError) {
                return ParseResult<Args>::error(parsedAddress.errorString);
            }

            index++;

            auto parsedTypes = TypesArgRequired<>::parse(args);

            if (parsedTypes.isError) {
                return ParseResult<Args>::error(parsedTypes.errorString);
            }

            index++;

            auto parsedSymbols = SymbolsArgRequired<>::parse(args);

            if (parsedSymbols.isError) {
                return ParseResult<Args>::error(parsedSymbols.errorString);
            }

            index++;

            bool fromTimeIsParsed{};
            std::optional<std::string> fromTime{};
            bool sourceIsParsed{};
            std::optional<std::string> source{};
            bool propertiesIsParsed{};
            std::optional<std::string> properties{};
            bool tapeIsParsed{};
            std::optional<std::string> tape{};
            bool isQuite{};
            bool forceStream{};

            while (index < args.size()) {
                if (!fromTimeIsParsed && FromTimeArg::canParse(args, index)) {
                    auto parseResult = FromTimeArg::parse(args, index);

                    fromTime = parseResult.result;
                    fromTimeIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!sourceIsParsed && SourceArg::canParse(args, index)) {
                    auto parseResult = SourceArg::parse(args, index);

                    source = parseResult.result;
                    sourceIsParsed = true;
                    index = parseResult.nextIndex;
                } else if (!propertiesIsParsed && PropertiesArg::canParse(args, index)) {
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
                    // ReSharper disable once CppUsingResultOfAssignmentAsCondition
                    if (!isQuite && (isQuite = QuiteArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    // ReSharper disable once CppUsingResultOfAssignmentAsCondition
                    if (!forceStream && (forceStream = ForceStreamArg::parse(args, index).result)) {
                        index++;
                        continue;
                    }

                    index++;
                }
            }

            return ParseResult<Args>::ok({parsedAddress.result, parsedTypes.result, parsedSymbols.result, fromTime,
                                          source, properties, tape, isQuite, forceStream});
        }
    };

    static void run(const Args &args) noexcept {
        try {
            using namespace std::literals;

            auto parsedProperties = CmdArgsUtils::parseProperties(args.properties);

            System::setProperties(parsedProperties);

            auto [parsedTypes, unknownTypes] = CmdArgsUtils::parseTypes(args.types);

            if (!unknownTypes.empty()) {
                auto unknown = elementsToString(unknownTypes.begin(), unknownTypes.end(), "", "");

                throw InvalidArgumentException(
                    fmt::format("There are unknown event types: {}!\n List of available event types: {}", unknown,
                                enum_utils::getEventTypeEnumClassNamesList(", ")));
            }

            if (parsedTypes.empty()) {
                throw InvalidArgumentException("The resulting list of types is empty!");
            }

            auto parsedSymbols = CmdArgsUtils::parseSymbols(args.symbols);

            if (args.fromTime.has_value()) {
                auto fromTime = TimeFormat::DEFAULT.parse(args.fromTime.value());

                parsedSymbols = parsedSymbols | ranges::views::transform([fromTime](const auto &sw) {
                                    return TimeSeriesSubscriptionSymbol{sw, fromTime};
                                }) |
                                ranges::to<std::unordered_set<SymbolWrapper>>;
            } else if (args.source.has_value()) {
                auto source = OrderSource::valueOf(args.source.value());

                parsedSymbols = parsedSymbols | ranges::views::transform([source](const auto &sw) {
                                    return IndexedEventSubscriptionSymbol{sw, source};
                                }) |
                                ranges::to<std::unordered_set<SymbolWrapper>>;
            }

            auto endpoint = DXEndpoint::newBuilder()
                                ->withRole(args.forceStream ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                                ->withProperties(parsedProperties)
                                ->withName(NAME + "Tool-Feed")
                                ->build();

            std::shared_ptr<DXFeedSubscription> sub = endpoint->getFeed()->createSubscription(parsedTypes);

            if (!args.isQuite) {
                sub->addEventListener([](auto &&events) {
                    for (auto &&e : events) {
                        std::cout << e << "\n";
                    }

                    std::cout.flush();
                });
            }

            if (args.tape.has_value()) {
                std::string tape = args.tape.value();

                auto pub = DXEndpoint::newBuilder()
                               ->withRole(DXEndpoint::Role::STREAM_PUBLISHER)
                               ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true") // Enabled by default
                               ->withProperties(parsedProperties)
                               ->withName(NAME + "Tool-Publisher")
                               ->build()
                               ->connect(tape.starts_with("tape:") ? tape : "tape:" + tape)
                               ->getPublisher();

                sub->addEventListener([pub](auto &&events) {
                    pub->publishEvents(events);
                });
            }

            sub->addSymbols(parsedSymbols);
            endpoint->connect(args.address);
            std::this_thread::sleep_for(std::chrono::days(365));
        } catch (const RuntimeException &e) {
            std::cerr << e << '\n';
        }
    }
};

} // namespace dxfcpp::tools