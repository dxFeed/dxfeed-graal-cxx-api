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

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;
    }
};

} // namespace dxfcpp::tools