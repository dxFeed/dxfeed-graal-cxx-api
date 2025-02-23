// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <utility>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

namespace dxfcpp::tools {

inline decltype(ranges::views::filter([](const auto &s) {
    return !s.empty();
})) filterNonEmpty{};

inline decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
})) transformToString{};

inline decltype(ranges::views::transform([](const std::string &s) {
    return trimStr(s);
})) trim{};

inline auto splitAndTrim = [](const std::string &s, char sep = ',') noexcept {
    return s | ranges::views::split(sep) | transformToString | trim;
};

template <typename R> struct ParseResult {
    static constexpr std::size_t LAST_INDEX{static_cast<std::size_t>(-1)};
    R result{};
    std::string errorString{};
    bool isError{};
    bool isHelp{};
    std::size_t nextIndex{LAST_INDEX};

    static ParseResult ok(R &&r, std::size_t nextIndex = LAST_INDEX) noexcept {
        return {r, "", false, false, nextIndex};
    }

    static ParseResult ok(const R &r, std::size_t nextIndex = LAST_INDEX) noexcept {
        return {r, "", false, false, nextIndex};
    }

    static ParseResult error(const std::string &errorString) noexcept {
        return {{}, errorString, true, false, LAST_INDEX};
    }

    static ParseResult help() noexcept {
        return {{}, {}, false, true, LAST_INDEX};
    }
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

template <std::size_t POSITION = 0> struct PositionalArg : Arg {
    static constexpr std::size_t getPosition() noexcept {
        return POSITION;
    }

    template <typename>
    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args) noexcept {
        return ParseResult<std::optional<std::string>>::ok(args.size() > POSITION ? std::optional{args[POSITION]}
                                                                                  : std::nullopt);
    }
};

struct RequiredMixin {
    template <typename A>
        requires requires { A::getPosition(); }
    static ParseResult<std::string> defaultParseImpl(const std::vector<std::string> &args) noexcept {
        return ParseResult<std::string>::ok(args[A::getPosition()]);
    }

    template <typename A, auto parseImpl = defaultParseImpl<A>>
        requires requires { A::getPosition(); }
    static ParseResult<std::string> parse(const std::vector<std::string> &args) noexcept {
        if (args.size() > A::getPosition()) {
            return parseImpl(args);
        }

        return ParseResult<std::string>::error("\"" + A::NAME + "\" argument parsing error. Insufficient parameters.");
    }
};

struct NamedArg : Arg {
    template <typename A> static bool canParse(const std::vector<std::string> &args, std::size_t index) noexcept {
        return args.size() > index + 1 && ((!A::SHORT_NAME.empty() && args[index] == "-" + A::SHORT_NAME) ||
                                           (!A::LONG_NAME.empty() && args[index] == "--" + A::LONG_NAME));
    }

    template <typename A>
    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args,
                                                         std::size_t index) noexcept {
        if (args.size() <= index + 1) {
            return ParseResult<std::optional<std::string>>::ok(std::nullopt, index);
        }

        if ((!A::SHORT_NAME.empty() && args[index] == "-" + A::SHORT_NAME) ||
            (!A::LONG_NAME.empty() && args[index] == "--" + A::LONG_NAME)) {
            return ParseResult<std::optional<std::string>>::ok(args[index + 1], index + 2);
        }

        return ParseResult<std::optional<std::string>>::ok(std::nullopt, index);
    }

    template <typename A> [[nodiscard]] static std::string getFullName() noexcept {
        std::string result{};

        if (!A::SHORT_NAME.empty()) {
            result += "-" + A::SHORT_NAME;
        }

        if (!A::LONG_NAME.empty()) {
            result += (!A::SHORT_NAME.empty() ? ", " : String::EMPTY) + "--" + A::LONG_NAME;
        }

        return result;
    }
};

struct NamedUnsignedIntArg : NamedArg {
    template <typename A>
    static ParseResult<std::optional<std::size_t>> parse(const std::vector<std::string> &args,
                                                         std::size_t index) noexcept {
        if (args.size() <= index + 1) {
            return ParseResult<std::optional<std::size_t>>::ok(std::nullopt, index);
        }

        if ((!A::SHORT_NAME.empty() && args[index] == "-" + A::SHORT_NAME) ||
            (!A::LONG_NAME.empty() && args[index] == "--" + A::LONG_NAME)) {
            try {
                const auto res = std::stoull(args[index + 1]);

                if (res >= std::numeric_limits<std::size_t>::max()) {
                    return ParseResult<std::optional<std::size_t>>::ok(std::nullopt, index);
                }

                return ParseResult<std::optional<std::size_t>>::ok(static_cast<std::size_t>(res), index + 2);
            } catch (...) {
                return ParseResult<std::optional<std::size_t>>::ok(std::nullopt, index);
            }
        }

        return ParseResult<std::optional<std::size_t>>::ok(std::nullopt, index);
    }
};

struct FlagArg : NamedArg {
    template <typename A>
    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) noexcept {
        auto r = ParseResult<bool>::ok(args.size() > index &&
                                           ((!A::SHORT_NAME.empty() && args[index] == "-" + A::SHORT_NAME) ||
                                            (!A::LONG_NAME.empty() && args[index] == "--" + A::LONG_NAME)),
                                       index);

        if (r.result) {
            r.nextIndex = index + 1;
        }

        return r;
    }
};

template <std::size_t POSITION = 0> struct TailArg : PositionalArg<POSITION>, RequiredMixin {
    template <typename A> static ParseResult<std::string> parse(const std::vector<std::string> &args) noexcept {
        return RequiredMixin::parse<A, [](const std::vector<std::string> &args) noexcept {
            return ParseResult<std::string>::ok(args | ranges::views::join(std::string(" ")) | ranges::to<std::string>,
                                                args.size() - 1);
        }>(args);
    }
};

template <std::size_t POSITION = 0> struct AddressArg : PositionalArg<POSITION> {
    const static std::string NAME;
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
        return PositionalArg<POSITION>::template parse<AddressArg>(args);
    }
};

template <std::size_t POSITION> const std::string AddressArg<POSITION>::NAME{"address"};

template <std::size_t POSITION>
const std::string AddressArg<POSITION>::HELP_TEXT{R"(
The address(es) to connect to retrieve data (see "Help address").
For Token-Based Authorization, use the following format: "<address>:<port>[login=entitle:<token>]".
)"};

template <std::size_t POSITION = 0> struct AddressArgRequired : AddressArg<POSITION>, RequiredMixin {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<AddressArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(AddressArg<POSITION>::HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        return RequiredMixin::parse<AddressArgRequired>(args);
    }
};

template <std::size_t POSITION = 1> struct TypesArg : PositionalArg<POSITION> {
    const static std::string NAME;
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
        return PositionalArg<POSITION>::template parse<TypesArg>(args);
    }
};

template <std::size_t POSITION> const std::string TypesArg<POSITION>::NAME{"types"};

template <std::size_t POSITION>
const std::string TypesArg<POSITION>::HELP_TEXT{R"(
Comma-separated list of dxfeed event types (e.g. Quote, TimeAndSale).
Use "all" for all available event types.
)"};

template <std::size_t POSITION = 1> struct TypesArgRequired : TypesArg<POSITION>, RequiredMixin {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<TypesArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(TypesArg<POSITION>::HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        return RequiredMixin::parse<TypesArgRequired>(args);
    }
};

template <std::size_t POSITION = 2> struct SymbolsArg : PositionalArg<POSITION> {
    const static std::string NAME;
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
        return PositionalArg<POSITION>::template parse<SymbolsArg>(args);
    }
};

template <std::size_t POSITION> const std::string SymbolsArg<POSITION>::NAME{"symbols"};

template <std::size_t POSITION>
const std::string SymbolsArg<POSITION>::HELP_TEXT{R"(
Comma-separated list of symbol names to get events for (e.g. "IBM, AAPL, MSFT").
Use "all" for wildcard subscription. The "dxfeed.wildcard.enable" property must be set to true to enable wildcard subscription.
A symbol filter set format can also be used as symbols source, for example: "ipf[https://demo:demo@tools.dxfeed.com/ipf?TYPE=STOCK]"
)"};

template <std::size_t POSITION = 2> struct SymbolsArgRequired : SymbolsArg<POSITION>, RequiredMixin {
    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<SymbolsArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(SymbolsArg<POSITION>::HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        if (args.size() > POSITION) {
            return ParseResult<std::string>::ok(args[POSITION]);
        }

        return ParseResult<std::string>::error("Symbols parsing error. Insufficient parameters.");
    }
};

struct PropertiesArg : NamedArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<PropertiesArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<PropertiesArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static bool canParse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::canParse<PropertiesArg>(args, index);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::parse<PropertiesArg>(args, index);
    }
};

struct FromTimeArg : NamedArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<FromTimeArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<FromTimeArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static bool canParse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::canParse<FromTimeArg>(args, index);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::parse<FromTimeArg>(args, index);
    }
};

struct SourceArg : NamedArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<SourceArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<SourceArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static bool canParse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::canParse<SourceArg>(args, index);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::parse<SourceArg>(args, index);
    }
};

struct TapeArg : NamedArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<TapeArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<TapeArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static bool canParse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::canParse<TapeArg>(args, index);
    }

    static ParseResult<std::optional<std::string>> parse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::parse<TapeArg>(args, index);
    }
};

struct QuiteArg : FlagArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<QuiteArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<QuiteArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) {
        return FlagArg::parse<QuiteArg>(args, index);
    }
};

struct ForceStreamArg : FlagArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<ForceStreamArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<ForceStreamArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) {
        return FlagArg::parse<ForceStreamArg>(args, index);
    }
};

struct CPUUsageByCoreArg : FlagArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<CPUUsageByCoreArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<CPUUsageByCoreArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) {
        return FlagArg::parse<CPUUsageByCoreArg>(args, index);
    }
};

struct DetachListenerArg : FlagArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<DetachListenerArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<DetachListenerArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) {
        return FlagArg::parse<DetachListenerArg>(args, index);
    }
};

struct IntervalArg : NamedUnsignedIntArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<IntervalArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<IntervalArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static bool canParse(const std::vector<std::string> &args, std::size_t index) {
        return NamedArg::canParse<IntervalArg>(args, index);
    }

    static ParseResult<std::optional<std::size_t>> parse(const std::vector<std::string> &args, std::size_t index) {
        return NamedUnsignedIntArg::parse<IntervalArg>(args, index);
    }
};

struct HelpArg : FlagArg {
    const static std::string NAME;
    const static std::string SHORT_NAME;
    const static std::string LONG_NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<HelpArg>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NamedArg::getFullName<HelpArg>();
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return trimStr(HELP_TEXT);
    }

    static ParseResult<bool> parse(const std::vector<std::string> &args, std::size_t index) {
        return FlagArg::parse<HelpArg>(args, index);
    }
};

template <std::size_t POSITION = 0> struct ArticleArgRequired : TailArg<POSITION> {
    const static std::string NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<ArticleArgRequired>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return fmt::format("{} (pos. {})", NAME, POSITION);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        return TailArg<POSITION>::template parse<ArticleArgRequired>(args);
    }
};

template <std::size_t POSITION> const std::string ArticleArgRequired<POSITION>::NAME{"article"};

template <std::size_t POSITION>
const std::string ArticleArgRequired<POSITION>::HELP_TEXT{R"(
Help article to show.
)"};

template <std::size_t POSITION = 0> struct QdsArgs : PositionalArg<POSITION>, RequiredMixin {
    const static std::string NAME;
    const static std::string HELP_TEXT;

    [[nodiscard]] static std::string getFullName() noexcept {
        return fmt::format("{} (pos. {})", NAME, POSITION);
    }

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t windowSize) noexcept {
        return Arg::prepareHelp<QdsArgs>(namePadding, nameFieldSize, windowSize);
    }

    [[nodiscard]] static std::string getFullHelpText() noexcept {
        return fmt::format("Required. {}", trimStr(HELP_TEXT));
    }

    static ParseResult<std::string> parse(const std::vector<std::string> &args) {
        return RequiredMixin::parse<QdsArgs>(args);
    }
};

template <std::size_t POSITION> const std::string QdsArgs<POSITION>::NAME{"qds-args"};

template <std::size_t POSITION>
const std::string QdsArgs<POSITION>::HELP_TEXT{R"(
Represents the arguments passed to the qds-tools.
)"};

using ArgType =
    std::variant<tools::AddressArg<>, tools::AddressArgRequired<>, tools::TypesArg<>, tools::TypesArgRequired<>,
                 tools::SymbolsArg<>, tools::SymbolsArgRequired<>, tools::SymbolsArgRequired<1>, tools::PropertiesArg,
                 tools::FromTimeArg, tools::SourceArg, tools::TapeArg, tools::QuiteArg, tools::ForceStreamArg,
                 tools::CPUUsageByCoreArg, tools::DetachListenerArg, tools::IntervalArg, tools::HelpArg,
                 tools::ArticleArgRequired<>, QdsArgs<>>;

} // namespace dxfcpp::tools