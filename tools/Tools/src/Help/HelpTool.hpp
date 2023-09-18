// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

namespace dxfcpp::tools {

struct HelpTool {
    static const std::unordered_map<std::string, std::string>
        EMBEDDED_ARTICLES;
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;
    }

    static std::string getArticle(const std::string& article) noexcept {
        for (auto&& [key, value] : EMBEDDED_ARTICLES) {
            if (iEquals(key, value)) {
                return value;
            }
        }

        return String::EMPTY;
    }

    template<typename Tool>
    static std::string generateHelpScreen() noexcept {
        auto [width, height] = Console::getSize();
        const static std::size_t PADDING = 2;

        std::string result{};

        result += "\n" + Tool::NAME + ((Tool::NAME.size() <= width - 1) ? "\n" : "") +
                  std::string(Tool::NAME.size() <= width - 1 ? Tool::NAME.size() : width - 1, '=') + "\n";
        result += Tool::DESCRIPTION.empty() ? Tool::SHORT_DESCRIPTION : Tool::DESCRIPTION + "\n";

        result += "Usage:\n";

        for (auto&& u : Tool::USAGE) {
            result += fmt::format("{:{}}{}\n", "", PADDING, u);
        }

        result += "\n";

        result += "Where:\n\n";

        auto maxFullNameSize = ranges::max(Tool::ARGS | ranges::views::transform([](auto &&arg) {
                                               return std::visit(
                                                   []<typename Arg>(Arg &&) {
                                                       return std::decay_t<Arg>::getFullName().size();
                                                   },
                                                   arg);
                                           }));

        for (auto &&arg : Tool::ARGS) {
            result += std::visit(
                [maxFullNameSize, width]<typename Arg>(Arg &&) {
                    return fmt::format("{}", std::decay_t<Arg>::prepareHelp(PADDING, PADDING + maxFullNameSize + PADDING, width));
                },
                arg);
        }

        auto article = HelpTool::getArticle(Tool::NAME);

        if (!article.empty()) {
            result += "\n" + article + "\n";
        }

        return result;
    }
};

} // namespace dxfcpp::tools