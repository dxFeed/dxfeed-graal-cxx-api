// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string>
#include <thread>
#include <cstdint>

namespace dxfcpp {

struct String {
    inline static const std::string EMPTY{};
};

std::string toString(bool b);

std::string toString(const char *chars);

std::string toString(std::thread::id theadId);

std::string toString(void* ptr);

char utf16to8(std::int16_t in);

std::int16_t utf8to16(char in);

std::string formatTimeStamp(std::int64_t timestamp);

std::string formatTimeStampWithMillis(std::int64_t timestamp);

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

std::string encodeChar(std::int16_t c);

inline std::string encodeChar(char c) { return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c))); }

}