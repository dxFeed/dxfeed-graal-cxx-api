// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define _CRT_SECURE_NO_WARNINGS

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4996)

#include <chrono>
#include <cstring>
#include <ctime>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <utf8.h>
#include <utility>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

#if !defined(__cpp_lib_chrono) || (__cpp_lib_chrono < 201907L)
#    include <date/date.h>
#endif

DXFCPP_BEGIN_NAMESPACE

auto getUtcOffset() {
    const std::time_t epochPlus11H = 60 * 60 * 11;
    const int localTime = localtime(&epochPlus11H)->tm_hour * 60 + localtime(&epochPlus11H)->tm_min;
    const int gmTime = gmtime(&epochPlus11H)->tm_hour * 60 + gmtime(&epochPlus11H)->tm_min;
    const int tzDiff = localTime - gmTime;

    return tzDiff * 60 * 1000;
}

decltype(ranges::views::filter([](const auto &s) {
    return !s.empty();
})) filterNonEmpty{};

decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
})) transformToString{};

auto splitAndTrim = [](auto &&symbols, char sep = ',') noexcept {
    decltype(ranges::views::transform([](const std::string &s) {
        return trimStr(s);
    })) trim{};

    return symbols | ranges::views::split(sep) | filterNonEmpty | transformToString | trim;
};

auto toUpper = [](auto &&s) {
    auto locale = std::locale{};

    return s | ranges::views::transform([&locale](auto c) {
               return std::toupper(c, locale);
           }) |
           ranges::to<std::string>();
};

std::unordered_set<std::string> parseStringSymbols(const std::string &symbols) noexcept {
    auto trimmedSymbols = trimStr(symbols);

    if (trimmedSymbols.empty()) {
        return {};
    }

    std::unordered_set<std::string> result{};

    auto addSymbol = [&result](auto &&symbol) {
        auto trim = trimStr(symbol);

        if (!trim.empty()) {
            result.emplace(trim);
        }
    };

    std::string symbol{};
    // Count of encountered parentheses of any type.
    auto parentheses = 0ULL;

    for (auto c : symbols) {
        switch (c) {
        case '{':
        case '(':
        case '[':
            ++parentheses;
            symbol += c;
            break;
        case '}':
        case ')':
        case ']':
            if (parentheses > 0) {
                --parentheses;
            }

            symbol += c;
            break;
        case ',':
            if (parentheses == 0) {
                // Not in parentheses -- comma is a symbol list separator.
                addSymbol(symbol);

                symbol.clear();
            } else {
                symbol += c;
            }

            break;
        default:
            symbol += c;
            break;
        }
    }

    addSymbol(symbol);

    return result;
}

std::unordered_set<SymbolWrapper> CmdArgsUtils::parseSymbols(const std::string &symbols) {
    auto trimmedSymbols = trimStr(symbols);

    if (trimmedSymbols.empty()) {
        return {};
    }

    auto parsed = isolated::internal::IsolatedTools::parseSymbols(trimmedSymbols);

    if (parsed.contains("*") || parsed.contains("all") || parsed.contains("All") || parsed.contains("ALL")) {
        return {WildcardSymbol::ALL};
    }

    return parsed | ranges::to<std::unordered_set<SymbolWrapper>>();
}

std::unordered_set<SymbolWrapper> CmdArgsUtils::parseSymbols(std::optional<std::string> symbols) noexcept {
    if (symbols.has_value()) {
        return parseSymbols(symbols.value());
    }

    return {};
}

std::unordered_set<CandleSymbol> CmdArgsUtils::parseCandleSymbols(const std::string &symbols) {
    auto trimmedSymbols = trimStr(symbols);

    if (trimmedSymbols.empty()) {
        return {};
    }

    auto parsed = isolated::internal::IsolatedTools::parseSymbols(trimmedSymbols);

    return parsed | ranges::views::transform([](auto &&s) {
               return CandleSymbol::valueOf(s);
           }) |
           ranges::to<std::unordered_set<CandleSymbol>>();
}

std::unordered_set<CandleSymbol> CmdArgsUtils::parseCandleSymbols(std::optional<std::string> symbols) noexcept {
    if (symbols.has_value()) {
        return parseCandleSymbols(symbols.value());
    }

    return {};
}

std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
CmdArgsUtils::parseTypes(const std::string &types) {
    auto trimmedTypes = trimStr(types);

    if (trimmedTypes.empty()) {
        return {};
    }

    if (trimmedTypes == "*" || trimmedTypes == "all" || trimmedTypes == "All" || trimmedTypes == "ALL") {
        return {EventTypeEnum::ALL | ranges::to<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>>(), {}};
    }

    auto split = splitAndTrim(trimmedTypes) | filterNonEmpty | ranges::to<std::vector<std::string>>;

    std::unordered_set<std::reference_wrapper<const EventTypeEnum>> result;
    std::vector<std::string> unknown;

    for (auto t : split) {
        auto u = toUpper(t);

        if (EventTypeEnum::ALL_BY_NAME.contains(u)) {
            result.emplace(EventTypeEnum::ALL_BY_NAME.at(u));
        } else if (EventTypeEnum::ALL_BY_CLASS_NAME.contains(t)) {
            result.emplace(EventTypeEnum::ALL_BY_CLASS_NAME.at(t));
        } else {
            unknown.push_back(t);
        }
    }

    return {result, unknown};
}

std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
CmdArgsUtils::parseTypes(std::optional<std::string> types) {
    if (types.has_value()) {
        return parseTypes(types.value());
    }

    return {};
}

std::unordered_map<std::string, std::string> CmdArgsUtils::parseProperties(const std::string &properties) noexcept {
    auto p = trimStr(properties);

    if (p.empty()) {
        return {};
    }

    return splitAndTrim(p) | filterNonEmpty | ranges::views::transform([](const std::string &kv) {
               return splitAndTrim(kv, '=') | ranges::to<std::vector<std::string>>();
           }) |
           ranges::views::filter([](auto &&kv) {
               return kv.size() == 2;
           }) |
           ranges::views::transform([](auto &&kv) {
               return std::make_pair(kv[0], kv[1]);
           }) |
           ranges::to<std::unordered_map<std::string, std::string>>();
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()