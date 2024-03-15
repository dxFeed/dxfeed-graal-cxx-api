// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include "../Connect/ConnectTool.hpp"
#include "../Dump/DumpTool.hpp"
#include "../LatencyTest/LatencyTestTool.hpp"
#include "../PerfTest/PerfTestTool.hpp"
#include "../Qds/QdsTool.hpp"

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

#include <console/console.hpp>

namespace dxfcpp::tools {

struct HelpTool {
    using Tool =
        std::variant<tools::ConnectTool, tools::DumpTool, tools::HelpTool, tools::LatencyTest, tools::PerfTestTool, tools::QdsTool>;

    static const std::unordered_map<std::string, std::string> EMBEDDED_ARTICLES;
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
        return fmt::format("{:{}}{:<{}}{:{}}{}\n", "", namePadding, getFullName(),
                           nameFieldSize - 2 * namePadding, "", namePadding, SHORT_DESCRIPTION);
    }

    static const std::unordered_map<std::string, Tool> ALL_TOOLS;
    static const std::vector<std::string> ALL_TOOL_NAMES;
    static const std::vector<std::string> ALL_ARTICLE_NAMES;
    static const std::set<std::string> ALL_NAMES;

    static constexpr std::size_t PADDING{2};

    struct Args {
        std::string article;

        static ParseResult<Args> parse(const std::vector<std::string> &args) noexcept {
            std::size_t index = 0;

            if (HelpArg::parse(args, index).result) {
                return ParseResult<Args>::help();
            }

            auto parsedArticle = ArticleArgRequired::parse(args);

            if (parsedArticle.isError) {
                return ParseResult<Args>::error(parsedArticle.errorString);
            }

            return ParseResult<Args>::ok({parsedArticle.result});
        }
    };

    static void run(const Args &args) noexcept {
        auto [width, height] = org::ttldtor::console::Console::getSize();
        using namespace std::literals;

        if (iEquals(args.article, "all")) {
            fmt::print("{0:-^{1}}\n", "", width - 1);
            for (auto&& name : ALL_NAMES) {
                std::cout << generateScreen(name) << std::endl;
                fmt::print("{0:-^{1}}\n", "", width - 1);
            }
        } else if (iEquals(args.article, "contents")) {
            fmt::print("\nHelp articles:\n");

            for (auto&& name : ALL_NAMES) {
                fmt::print("{:{}}{}\n", "", PADDING, name);
            }
        } else {
            auto screen = generateScreen(args.article);

            if (screen.empty()) {
                fmt::print("\n{:{}}No help article found for \"{}\".\n", "", PADDING, args.article);
            } else {
                fmt::print("{}\n", screen);
            }
        }
    }

    static std::pair<std::string, std::string> getArticle(const std::string &article) noexcept {
        for (auto &&[key, value] : EMBEDDED_ARTICLES) {
            if (iEquals(key, article)) {
                return {key, value};
            }
        }

        return {String::EMPTY, String::EMPTY};
    }

    template <typename Tool>
    static std::string generateHeader(std::size_t width, const std::string &parseResult = "") noexcept {
        auto toolName = Tool::getName();

        std::string result{};

        result = fmt::format("\n{}{}", toolName, ((toolName.size() <= width - 1) ? "\n" : "")) +
                 fmt::format("{0:=^{1}}\n", "", (toolName.size() <= width - 1 ? toolName.size() : width - 1));

        if (!parseResult.empty()) {
            result += fmt::format("\n{:{}}{}\n", "", PADDING, parseResult);
        } else if (!Tool::DESCRIPTION.empty()) {
            result += Tool::DESCRIPTION;
        } else if (!Tool::SHORT_DESCRIPTION.empty()) {
            result += "\n" + Tool::SHORT_DESCRIPTION + "\n";
        }

        return result;
    }

    template <typename Tool> static std::string generateArgsInfo(std::size_t width) noexcept {
        std::string result{};

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
                    return fmt::format(
                        "{}", std::decay_t<Arg>::prepareHelp(PADDING, PADDING + maxFullNameSize + PADDING, width));
                },
                arg);
        }

        return result;
    }

    template <typename Tool> static std::string generateUsage() noexcept {
        std::string result = "Usage:\n";

        for (auto &&u : Tool::USAGE) {
            result += fmt::format("{:{}}{}\n", "", PADDING, u);
        }

        return result;
    }

    template <typename Tool> static std::string generateAdditionalInfo() noexcept {
        std::string result{};

        if (!Tool::ADDITIONAL_INFO.empty()) {
            result += "\n";

            for (auto &&ai : Tool::ADDITIONAL_INFO) {
                result += ai + "\n";
            }
        }

        return result;
    }

    template <typename Tool> static std::string generateArticleInfo() noexcept {
        std::string result{};
        auto [article, content] = HelpTool::getArticle(Tool::getName());

        if (!content.empty()) {
            result += "\n" + content + "\n";
        }

        return result;
    }

    template <typename Tool> static std::string generateToolHelpScreen(const std::string &parseResult = "") noexcept {
        auto [width, height] = org::ttldtor::console::Console::getSize();

        std::string result{};

        result += generateHeader<Tool>(width, parseResult) + "\n";
        result += generateUsage<Tool>();
        result += "\nWhere:\n\n";
        result += generateArgsInfo<Tool>(width);
        result += generateAdditionalInfo<Tool>();
        result += generateArticleInfo<Tool>();

        return result;
    }

    static std::string generateArticleHeader(std::size_t width, const std::string &article) noexcept {
        std::string result{};

        result = fmt::format("\n{}{}", article, ((article.size() <= width - 1) ? "\n" : "")) +
                 fmt::format("{0:=^{1}}\n", "", (article.size() <= width - 1 ? article.size() : width - 1));

        return result;
    }

    static std::string generateArticleScreen(const std::string &article, const std::string& content) noexcept {
        auto [width, height] = org::ttldtor::console::Console::getSize();
        std::string result{};

        result += generateArticleHeader(width, article) + "\n";
        result += content + "\n";

        return result;
    }

    static std::string generateScreen(const std::string& article) noexcept {
        std::string screen{};

        for (auto&& [name, tool] : ALL_TOOLS) {
            if (iEquals(article, name)) {
                screen = std::visit(
                    []<typename Tool>(Tool &&) {
                        using T = std::decay_t<Tool>;

                        return tools::HelpTool::generateToolHelpScreen<T>();
                    },
                    tool);

                break;
            }
        }

        if (!screen.empty()) {
            return screen;
        }

        auto [name, content] = getArticle(article);

        if (!content.empty()) {
            return generateArticleScreen(name, content);
        }

        return screen;
    }
};

} // namespace dxfcpp::tools