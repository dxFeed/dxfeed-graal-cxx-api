// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
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

struct Arg {
    template <typename A>
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        auto nameField = fmt::format("{:{}}{:<{}}{:{}}", "", namePadding, A::getFullName(),
                                     nameFieldSize - 2 * namePadding, "", namePadding);

        auto helpTextSize = windowSize - nameFieldSize - 1;
        auto fullHelpTextLines = splitAndTrim(A::getFullHelpText(), '\n') |
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
};

struct PositionalArg : Arg {};

template <typename R> struct ParseResult {
    R result{};
    std::string errorString{};
    bool isError{};

    static ParseResult ok(R &&r) noexcept {
        return {r, "", false};
    }

    static ParseResult ok(const R &r) noexcept {
        return {r, "", false};
    }

    static ParseResult error(const std::string &errorString) {
        return {{}, errorString, true};
    }
};

struct AddressArg : PositionalArg {
    const static std::string NAME;
    const static std::size_t POSITION{0};
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<AddressArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return fmt::format("{} (pos. {})", NAME, POSITION);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args) {
        return ParseResult<std::optional<std::string>>::ok(args.size() > POSITION ? std::optional{args[POSITION]}
                                                                                  : std::nullopt);
    }
};

struct AddressArgRequired : AddressArg {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<AddressArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        if (args.size() > POSITION) {
            return ParseResult<std::string>::ok(args[POSITION]);
        }

        return ParseResult<std::string>::error("Address parsing error. Insufficient parameters.");
    }
};

struct TypesArg : PositionalArg {
    const static std::string NAME;
    const static std::size_t POSITION{1};
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<TypesArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return fmt::format("{} (pos. {})", NAME, POSITION);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args) {
        return ParseResult<std::optional<std::string>>::ok(args.size() > POSITION ? std::optional{args[POSITION]}
                                                                                  : std::nullopt);
    }
};

struct TypesArgRequired : TypesArg {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<TypesArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        if (args.size() > POSITION) {
            return ParseResult<std::string>::ok(args[POSITION]);
        }

        return ParseResult<std::string>::error("Types parsing error. Insufficient parameters.");
    }
};

struct SymbolsArg : PositionalArg {
    const static std::string NAME;
    const static std::size_t POSITION{2};
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<SymbolsArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return fmt::format("{} (pos. {})", NAME, POSITION);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args) {
        return ParseResult<std::optional<std::string>>::ok(args.size() > POSITION ? std::optional{args[POSITION]}
                                                                                  : std::nullopt);
    }
};

struct SymbolsArgRequired : SymbolsArg {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<SymbolsArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        if (args.size() > POSITION) {
            return ParseResult<std::string>::ok(args[POSITION]);
        }

        return ParseResult<std::string>::error("Symbols parsing error. Insufficient parameters.");
    }
};




} // namespace dxfcpp::tools