// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <cstdint>
#include <string>
#include <thread>

namespace dxfcpp {

struct DXFCPP_EXPORT String {
    inline static const std::string EMPTY{};
};

DXFCPP_EXPORT std::string toString(bool b);

DXFCPP_EXPORT std::string toString(const char *chars);

DXFCPP_EXPORT std::string toString(std::thread::id theadId);

DXFCPP_EXPORT std::string toString(void *ptr);

template <typename T> std::string toStringAny(T &&t) {
    if constexpr (requires { t.toString(); }) {
        return t.toString();
    } else if constexpr (requires { *t.toString(); }) {
        return *t.toString();
    } else if constexpr (requires { toString(t); }) {
        return toString(t);
    } else if constexpr (requires { std::to_string(t); }) {
        return std::to_string(t);
    } else if constexpr (requires { std::string(t); }) {
        return std::string(t);
    } else {
        return "unknown";
    }
}

DXFCPP_EXPORT char utf16to8(std::int16_t in);

DXFCPP_EXPORT std::int16_t utf8to16(char in);

DXFCPP_EXPORT std::string formatTimeStamp(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithMillis(std::int64_t timestamp);

template <typename It>
#if __cpp_concepts
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> getName())>, std::string>; }
#endif
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + "'" + it->getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
std::string elementsToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + "'" + toStringAny(*it) + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

DXFCPP_EXPORT std::string encodeChar(std::int16_t c);

inline std::string encodeChar(char c) { return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c))); }

} // namespace dxfcpp