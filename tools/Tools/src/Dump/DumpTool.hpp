// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>

namespace dxfcpp::tools {

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


    struct Arg{};


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