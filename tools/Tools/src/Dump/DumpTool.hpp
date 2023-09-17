// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

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

decltype(ranges::views::filter([](const auto &s) {
    return !s.empty();
})) filterNonEmpty{};

decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
})) transformToString{};

decltype(ranges::views::transform([](const std::string &s) {
    return trimStr(s);
})) trim{};

inline auto splitAndTrim = [](const std::string &s, char sep = ',') noexcept {
    return s | ranges::views::split(sep) | transformToString | trim;
};

struct DumpTool {
    [[nodiscard]] std::string getName() const noexcept {
        return "Dump";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return "Dumps all events received from address.";
    }

    [[nodiscard]] std::string getDescription() const noexcept {
        return R"(
Dumps all events received from address.
Enforces a streaming contract for subscription. A wildcard enabled by default.
This was designed to receive data from a file.
If <types> is not specified, creates a subscription for all available event types.
If <symbol> is not specified, the wildcard symbol is used.
)";
    }

    [[nodiscard]] std::vector<std::string> getUsage() const noexcept {
        return {
            getName() + " <address> [<options>]",
            getName() + " <address> <types> [<options>]",
            getName() + " <address> <types> <symbols> [<options>]",
        };
    }

    [[nodiscard]] std::vector<std::string> getAdditionalInfo() const noexcept {
        return {};
    }

    struct Arg {};

    struct PositionalArg {};

    struct AddressArg : PositionalArg {
        std::string name{"address"};
        std::size_t position{0};
        std::string helpText{R"(
The address(es) to connect to retrieve data (see "Help address").
For Token-Based Authorization, use the following format: "<address>:<port>[login=entitle:<token>]".
)"};
        bool required{true};

        [[nodiscard]] std::string prepareHelp(std::size_t namePadding,
                                              std::size_t nameFieldSize /* padding + name + padding */,
                                              std::size_t windowSize) const noexcept {
            auto nameField = fmt::format("{:{}}{:<{}}{:{}}", "", namePadding, getFullName(),
                                         nameFieldSize - 2 * namePadding, "", namePadding);

            // std::cout << nameField << std::endl;

            auto helpTextSize = windowSize - nameFieldSize - 1;
            auto fullHelpTextLines = splitAndTrim(getFullHelpText(), '\n') |
                                     ranges::views::transform([helpTextSize](auto &&line) {
                                         if (line.size() > helpTextSize) {
                                             return line | ranges::views::chunk(helpTextSize) | transformToString |
                                                    ranges::to<std::vector<std::string>>();
                                         }

                                         return std::vector<std::string>{{line}};
                                     }) |
                                     ranges::views::join | ranges::to<std::vector<std::string>>();

            if (fullHelpTextLines.empty()) {
                return nameField + "\n";
            }

            std::string result{};

            for (std::size_t i = 0; i < fullHelpTextLines.size(); i++) {
                if (i == 0) {
                    result += fmt::format("{}{}\n", nameField, fullHelpTextLines[i]);
                } else if (fullHelpTextLines[i].empty()) {
                    result += "\n";
                } else {
                    result += fmt::format("{:{}}{}\n", "", nameFieldSize, fullHelpTextLines[i]);
                }
            }

            return result;
        }

        [[nodiscard]] std::string getFullName() const noexcept {
            return fmt::format("{} (pos. {})", name, position);
        }

        [[nodiscard]] std::string getFullHelpText() const noexcept {
            return fmt::format("{}{}", required ? "Required. " : "", trimStr(helpText));
        }
    };

    struct Args {
        std::string address;
        std::optional<std::string> types;
        std::optional<std::string> symbols;
        std::optional<std::string> properties;
        std::optional<std::string> tape;
        bool isQuite;

        static Args parse(const std::vector<std::string> &args) noexcept {
            return {};
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
                ->withName(getName() + "Tool")
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
                    ->withName(getName() + "Tool")
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