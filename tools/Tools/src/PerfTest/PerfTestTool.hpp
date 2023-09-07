// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <chrono>

namespace dxfcpp::tools {

struct PerfTestTool {
    [[nodiscard]] std::string getName() const noexcept {
        return "PerfTest";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return "Connects to specified address and calculates performance counters.";
    }

    [[nodiscard]] std::string getDescription() const noexcept {
        return R"(
Connects to the specified address(es) and calculates performance counters (events per second, cpu usage, etc).
)";
    }

    [[nodiscard]] std::vector<std::string> getUsage() const noexcept {
        return {
            getName() + " <address> <types> <symbols> [<options>]",
        };
    }

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;

        auto endpoint = DXEndpoint::newBuilder()
                            ->withRole(args.isForceStream() ? DXEndpoint::Role::STREAM_FEED : DXEndpoint::Role::FEED)
                            ->withProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true")
                            ->withProperties(CmdArgsUtils::parseProperties(args.getProperties()))
                            ->withName("PerfTestTool")
                            ->build();

        auto sub = endpoint->getFeed()->createSubscription(CmdArgsUtils::parseTypes(args.getTypes()));

        auto measurementPeriod = 2s;
    }
};

} // namespace dxfcpp::tools