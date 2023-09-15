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

struct ConnectTool {
    [[nodiscard]] std::string getName() const noexcept {
        return "Connect";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return "Connects to specified address(es).";
    }

    [[nodiscard]] std::string getDescription() const noexcept {
        return "Connects to the specified address(es) and subscribes to the specified symbols.";
    }

    [[nodiscard]] std::vector<std::string> getUsage() const noexcept {
        return {
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