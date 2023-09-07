// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <cstring>
#include <ctime>
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

#include <range/v3/all.hpp>

#if !defined(__cpp_lib_chrono) || (__cpp_lib_chrono < 201907L)
#    include <date/date.h>
#endif

namespace dxfcpp {

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

decltype(ranges::views::transform([](const std::string &s) {
    return trimStr(s);
})) trim{};

auto splitAndTrim = [](auto &&symbols, char sep = ',') noexcept {
    return symbols | ranges::views::split(sep) | filterNonEmpty | transformToString | trim;
};

decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::views::transform([](unsigned char c) {
               return std::toupper(c);
           }) |
           ranges::to<std::string>();
})) transformToUpper{};

std::unordered_set<std::string> CmdArgsUtils::parseSymbols(const std::string &symbols) noexcept {
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

std::unordered_set<std::reference_wrapper<const EventTypeEnum>>
CmdArgsUtils::parseTypes(const std::string &types) noexcept {
    auto split = splitAndTrim(types) | filterNonEmpty;
    auto allByName = split | transformToUpper | ranges::views::filter([](const auto &s) {
                         return EventTypeEnum::ALL_BY_NAME.contains(s);
                     }) |
                     ranges::views::transform([](const auto &s) {
                         return EventTypeEnum::ALL_BY_NAME.at(s);
                     });
    auto allByClassName = split | ranges::views::filter([](const auto &s) {
                              return EventTypeEnum::ALL_BY_CLASS_NAME.contains(s);
                          }) |
                          ranges::views::transform([](const auto &s) {
                              return EventTypeEnum::ALL_BY_CLASS_NAME.at(s);
                          });

    return ranges::views::concat(allByName, allByClassName) |
           ranges::to<std::unordered_set<std::reference_wrapper<const EventTypeEnum>>>();
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

std::int64_t parseDateTime(const std::string &string) noexcept {
    const static auto dateFormats = {
        "%Y%m%d", "%Y-%m-%d",
        //        "%Y\\%m\\%d",
        //        "%Y/%m/%d",
    };

    const static auto timeFormats = {
        "%H%M%S",
        "%H:%M:%S",
        "",
    };

    const static auto dateTimeSeparators = {
        " ", "-", "T",
        //        ""
    };

    const static auto timeZoneFormats = {
        "%z",
        "Z",
        "",
    };

    const static auto formats = [&] {
        std::vector<std::string> result{};

        result.reserve(dateFormats.size() *
                       ((timeFormats.size() - 1) * dateTimeSeparators.size() + 1 /* empty time format */) *
                       timeZoneFormats.size());

        for (auto df : dateFormats) {
            for (auto tf : timeFormats) {
                if (tf[0] != '\0') {
                    for (auto sep : dateTimeSeparators) {
                        for (auto tz : timeZoneFormats) {
                            result.emplace_back(std::string{df} + sep + tf + tz);
                        }
                    }
                } else {
                    for (auto tz : timeZoneFormats) {
                        result.emplace_back(std::string{df} + tz);
                    }
                }
            }
        }

        return result;
    }();

#if !defined(__cpp_lib_chrono) || (__cpp_lib_chrono < 201907L)
    using sys_millis = date::sys_time<std::chrono::milliseconds>;
    using local_millis = date::local_time<std::chrono::milliseconds>;
#else
    using sys_millis = std::chrono::sys_time<std::chrono::milliseconds>;
    using local_millis = std::chrono::local_time<std::chrono::milliseconds>;
#endif

    auto tryParse = []<typename TimePoint>(const std::string &format, const std::string &str,
                                           TimePoint tp = {}) -> std::int64_t {
        std::istringstream in{str};
        std::string abbrev{};
        std::chrono::minutes offset{};
        std::int64_t offsetMillis{};

#if !defined(__cpp_lib_chrono) || (__cpp_lib_chrono < 201907L)
        date::from_stream(in, format.c_str(), tp, &abbrev, &offset);
#else
        std::chrono::from_stream(in, format.c_str(), tp, &abbrev, &offset);
#endif

        if (in.fail()) {
            return -1LL;
        }

        offsetMillis = std::chrono::duration_cast<std::chrono::milliseconds>(offset).count();

        if (format.ends_with('Z')) { // offsetMillis == 0
            return tp.time_since_epoch().count();
        } else if (offsetMillis == 0 && !format.ends_with('z')) {
            return tp.time_since_epoch().count() - getUtcOffset();
        }

        return tp.time_since_epoch().count() - offsetMillis;
    };

    auto s = trimStr(string);

    if (s.empty()) {
        return -1;
    }

    if (s[0] == '0') {
        return 0;
    }

    try {
        for (const auto &f : formats) {
            if (auto r = tryParse(f, s, local_millis{}); r != -1) {
                return r;
            }
        }
    } catch (...) {
    }

    try {
        return std::stoll(s);
    } catch (...) {
    }

    return -1;
}

} // namespace dxfcpp
