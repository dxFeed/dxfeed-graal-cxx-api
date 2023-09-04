// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <utf8.h>
#include <utility>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>


namespace dxfcpp {

auto splitAndTrim = [](auto &&symbols) noexcept {
    return symbols | ranges::views::split(',') | ranges::views::filter([](const auto &s) {
               return !s.empty();
           }) |
           ranges::views::transform([](auto &&s) {
               return s | ranges::to<std::string>();
           }) |
           ranges::views::transform([](const std::string &s) {
               return trimStr(s);
           }) |
           ranges::views::filter([](const auto &s) {
               return !s.empty();
           });
};

std::unordered_set<std::string> CmdArgsUtils::parseSymbols(const std::string &symbols) {
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

std::unordered_set<std::reference_wrapper<const EventTypeEnum>> CmdArgsUtils::parseTypes(const std::string &types) {
    return splitAndTrim(types) | ranges::views::transform([](auto &&s) {
               return s | ranges::views::transform([](unsigned char c) {
                          return std::toupper(c);
                      }) |
                      ranges::to<std::string>();
           }) |
           ranges::views::filter([](const auto &s) {
               return EventTypeEnum::ALL_BY_NAME.contains(s);
           }) |
           ranges::views::transform([](const auto &s) {
               return EventTypeEnum::ALL_BY_NAME.at(s);
           }) |
           ranges::to<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>>();
}

} // namespace dxfcpp
