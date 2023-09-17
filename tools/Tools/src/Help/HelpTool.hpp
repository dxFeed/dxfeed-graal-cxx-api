// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace dxfcpp::tools {

struct HelpTool {
    static const std::unordered_map<std::string, std::string, decltype(&icHash), decltype(&iEquals)>
        EMBEDDED_ARTICLES;

    [[nodiscard]] std::string getName() const noexcept {
        return "Help";
    }

    [[nodiscard]] std::string getShortDescription() const noexcept {
        return "Help tool.";
    }

    [[nodiscard]] std::string getDescription() const noexcept {
        return "Displays documentation pages.";
    }

    [[nodiscard]] std::vector<std::string> getUsage() const noexcept {
        return {
            getName() + " <article>",
        };
    }

    [[nodiscard]] std::vector<std::string> getAdditionalInfo() const noexcept {
        return {
            R"(To see help on some topic type "Help <topic>".)",
            R"(To see list of all articles type "Help contents".)",
            R"(Use "Help all" to generate all existing help articles.)",
        };
    }

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;
    }
};

} // namespace dxfcpp::tools