// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()

DXFCPP_BEGIN_NAMESPACE

std::string toString(bool b) noexcept {
    return b ? "true" : "false";
}

std::string toString(const char *chars) noexcept {
    // TODO: cache [EN-8231]

    if (chars == nullptr) {
        return dxfcpp::String::NUL;
    }

    return chars;
}

std::optional<std::string> toStringOpt(const char *chars) noexcept {
    // TODO: cache [EN-8231]

    if (chars == nullptr) {
        return std::nullopt;
    }

    return chars;
}

std::string toString(std::thread::id threadId) {
    std::ostringstream result{};

    result << threadId;

    return result.str();
}

std::string toString(void *ptr) {
    std::ostringstream result{};

    result << ptr;

    return result.str();
}

std::string toString(double d) {
    if (std::isnan(d)) {
        return "NaN";
    }

    auto x = fmt::format("{}", d);
    auto y = fmt::format("{}", std::round(d));

    return x.size() == y.size() ? x + ".0" : x;
}

std::string encodeChar(std::int16_t c) {
    if (c >= 32 && c <= 126) {
        return std::string{} + static_cast<char>(c);
    }

    if (c == 0) {
        return "\\0";
    }

    return fmt::format("\\u{:04x}", c);
}

char utf16to8(std::int16_t in) noexcept {
    try {
        std::string out{};
        auto utf16in = {in};

        utf8::utf16to8(std::begin(utf16in), std::end(utf16in), std::back_inserter(out));

        return out.empty() ? char{} : out[0];
    } catch (...) {
        // TODO: error handling
        return char{};
    }
}

std::string utf16toUtf8String(std::int16_t in) noexcept {
    try {
        std::string out{};
        auto utf16in = {in};

        utf8::utf16to8(std::begin(utf16in), std::end(utf16in), std::back_inserter(out));

        return out;
    } catch (...) {
        // TODO: error handling
        return "";
    }
}

std::string utf16toUtf8String(const std::u16string &in) noexcept {
    try {
        std::string out{};

        utf8::utf16to8(std::begin(in), std::end(in), std::back_inserter(out));

        return out;
    } catch (...) {
        // TODO: error handling
        return "";
    }
}

std::string utf16toUtf8String(const std::vector<std::int16_t> &in) noexcept {
    try {
        std::string out{};

        utf8::utf16to8(std::begin(in), std::end(in), std::back_inserter(out));

        return out;
    } catch (...) {
        // TODO: error handling
        return "";
    }
}

std::int16_t utf8to16(char in) noexcept {
    try {
        std::u16string out{};
        auto utf8in = {in};

        utf8::utf8to16(std::begin(utf8in), std::end(utf8in), std::back_inserter(out));

        return out.empty() ? std::int16_t{} : static_cast<std::int16_t>(out[0]);
    } catch (...) {
        // TODO: error handling
        return std::int16_t{};
    }
}

std::string formatTimeStamp(std::int64_t timestamp) {
    return TimeFormat::DEFAULT.format(timestamp);
}

std::string formatTimeStampWithTimeZone(std::int64_t timestamp) {
    if (timestamp == 0) {
        return "0";
    }

    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%Y%m%d-%H%M%S%z}", tm);
}

std::string formatTimeStampWithMillis(std::int64_t timestamp) {
    return TimeFormat::DEFAULT_WITH_MILLIS.format(timestamp);
}

std::string formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp) {
    return TimeFormat::DEFAULT_WITH_MILLIS_WITH_TIMEZONE.format(timestamp);
}

char *createCString(const std::string &s) {
    if (s == dxfcpp::String::NUL) {
        return nullptr;
    }

    char *cString = new char[s.size() + 1];

    std::copy(s.begin(), s.end(), cString);
    cString[s.size()] = '\0';

    return cString;
}

DXFCPP_EXPORT char *createCString(const std::optional<std::string> &s) {
    if (!s) {
        return nullptr;
    }

    return createCString(s.value());
}

std::string trimStr(const std::string &s) noexcept {
    auto trimPredicate = [](auto c) {
        return c == ' ' || c == '\t' || c == '\v' || c == '\r' || c == '\n';
    };

    return s | ranges::views::drop_while(trimPredicate) | ranges::views::reverse |
           ranges::views::drop_while(trimPredicate) | ranges::views::reverse | ranges::to<std::string>();
};

// inline decltype(ranges::views::filter([](const auto &s) {
//     return !s.empty();
// })) filterNonEmpty{};

inline decltype(ranges::views::transform([](auto &&s) {
    return s | ranges::to<std::string>();
})) transformToString{};

// inline decltype(ranges::views::transform([](const std::string &s) {
//     return trimStr(s);
// })) trim{};

// inline auto splitAndTrim = [](const std::string &s, char sep = ',') noexcept {
//     return s | ranges::views::split(sep) | transformToString | trim;
// };

inline auto split = [](const std::string &s, char sep = ',') noexcept {
    return s | ranges::views::split(sep) | transformToString;
};

std::vector<std::string> splitStr(const std::string &s, char sep) noexcept {
    return split(s, sep) | ranges::to<std::vector<std::string>>();
}

std::string joinStr(const std::vector<std::string> &v, const std::string &sep) noexcept {
    return fmt::format("{}", fmt::join(v, sep));
}

bool toBool(const std::string &s) noexcept {
    return iEquals(s, "true") || iEquals(s, "yes") || iEquals(s, "y") || iEquals(s, "on");
}

DXFCPP_END_NAMESPACE