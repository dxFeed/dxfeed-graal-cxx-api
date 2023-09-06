// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
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

#include <date/date.h>

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
    auto split = splitAndTrim(types);
    auto allByName = split | ranges::views::transform([](auto &&s) {
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

std::int64_t parseDateTimeOrPeriod(const std::string &string) {
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

    using sys_millis = date::sys_time<std::chrono::milliseconds>;
    using local_millis = date::local_time<std::chrono::milliseconds>;

    auto tryParse = []<typename TimePoint>(const std::string &format, const std::string &str,
                                           TimePoint tp = {}) -> std::int64_t {
        std::istringstream in{str};
        date::from_stream(in, format.c_str(), tp);

        if (in.fail()) {
            return -1LL;
        }

        return tp.time_since_epoch().count();
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
            if (f.ends_with('Z')) {
                if (auto r = tryParse(f, s, sys_millis{}); r != -1) {
                    std::cerr << "sys: '" + f + "' -> '" + s + "'" << std::endl;

                    return r;
                }
            } else {
                if (auto r = tryParse(f, s, local_millis{}); r != -1) {
                    std::cerr << "local: '" + f + "' -> '" + s + "'" << std::endl;

                    return r;
                }
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
