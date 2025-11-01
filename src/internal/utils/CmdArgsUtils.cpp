// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define _CRT_SECURE_NO_WARNINGS

#include <dxfeed_graal_cpp_api/api.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedTools.hpp>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4996)

#include <ctime>
#include <locale>
#include <string>
#include <utility>
#include <vector>

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wunused-variable")
DXFCXX_DISABLE_GCC_WARNINGS("-Wmaybe-uninitialized")
DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wdeprecated-declarations")
#include <range/v3/all.hpp>
DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_MSC_WARNINGS_POP()

#if !defined(__cpp_lib_chrono) || (__cpp_lib_chrono < 201907L)
#    include <date/date.h>
#endif

DXFCPP_BEGIN_NAMESPACE

inline auto getUtcOffset() {
    constexpr std::time_t epochPlus11H = 60 * 60 * 11;
    const int localTime = localtime(&epochPlus11H)->tm_hour * 60 + localtime(&epochPlus11H)->tm_min;
    const int gmTime = gmtime(&epochPlus11H)->tm_hour * 60 + gmtime(&epochPlus11H)->tm_min;
    const int tzDiff = localTime - gmTime;

    return tzDiff * 60 * 1000;
}

inline auto filterNonEmpty = ranges::views::filter([](const auto &s) {
    return !s.empty();
});

inline auto transformToString = ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
});

inline auto splitAndTrim = [](auto &&symbols, char sep = ',') noexcept {
    decltype(ranges::views::transform([](const std::string &s) {
        return trimStr(s);
    })) trim{};

    return symbols | ranges::views::split(sep) | filterNonEmpty | transformToString | trim;
};

inline auto toUpper = [](auto &&s) {
    auto locale = std::locale{};

    return s | ranges::views::transform([&locale](auto c) {
               return std::toupper(c, locale);
           }) |
           ranges::to<std::string>();
};

std::unordered_set<SymbolWrapper> CmdArgsUtils::parseSymbols(const StringLike &symbols) {
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

std::vector<SymbolWrapper> CmdArgsUtils::parseSymbolsAndSaveOrder(const StringLike &symbols) {
    auto trimmedSymbols = trimStr(symbols);

    if (trimmedSymbols.empty()) {
        return {};
    }

    auto parsed = isolated::internal::IsolatedTools::parseSymbolsAndSaveOrder(trimmedSymbols);

    for (const auto &symbol : parsed) {
        if (symbol == "*" || iEquals(symbol, "all")) {
            return {WildcardSymbol::ALL};
        }
    }

    return parsed | ranges::to<std::vector<SymbolWrapper>>();
}

std::unordered_set<CandleSymbol> CmdArgsUtils::parseCandleSymbols(const StringLike &symbols) {
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

std::pair<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>, std::vector<std::string>>
CmdArgsUtils::parseTypes(const StringLike &types) {
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

std::unordered_map<std::string, std::string> CmdArgsUtils::parseProperties(const StringLike &properties) noexcept {
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

std::unordered_set<EventSourceWrapper> CmdArgsUtils::parseEventSources(const StringLike &sources) noexcept {
    auto s = trimStr(sources);

    if (s.empty()) {
        return {EventSourceWrapper{IndexedEventSource::DEFAULT}};
    }

    return splitAndTrim(s) | filterNonEmpty | ranges::views::transform([](auto &&name) {
               return EventSourceWrapper{OrderSource::valueOf(name)};
           }) |
           ranges::to<std::unordered_set<EventSourceWrapper>>;
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
DXFCXX_DISABLE_GCC_WARNINGS_POP()