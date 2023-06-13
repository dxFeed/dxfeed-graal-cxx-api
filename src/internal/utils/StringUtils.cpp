// Copyright (c) 2023 Devexperts LLC.
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
#include <fmt/std.h>

namespace dxfcpp {

std::string toString(bool b) { return b ? "true" : "false"; }

std::string toString(const char *chars) {
    if (chars == nullptr) {
        return "";
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

char utf16to8(std::int16_t in) {
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

std::int16_t utf8to16(char in) {
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
    if (timestamp == 0) {
        return "0";
    }

    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%Y%m%d-%H%M%S}", tm);
}

std::string formatTimeStampWithTimeZone(std::int64_t timestamp) {
    if (timestamp == 0) {
        return "0";
    }

    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%Y%m%d-%H%M%S%z}", tm);
}

std::string formatTimeStampWithMillis(std::int64_t timestamp) {
    if (timestamp == 0) {
        return "0";
    }

    auto ms = timestamp % 1000;
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%Y%m%d-%H%M%S}.{:0>3}", tm, ms);
}

std::string formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp) {
    if (timestamp == 0) {
        return "0";
    }

    auto ms = timestamp % 1000;
    auto tm = fmt::localtime(static_cast<std::time_t>(timestamp / 1000));

    return fmt::format("{:%Y%m%d-%H%M%S}.{:0>3}{:%z}", tm, ms, tm);
}

} // namespace dxfcpp