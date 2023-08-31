// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <cstddef>
#include <cstdint>
#include <locale>
#include <string>
#include <thread>

namespace dxfcpp {

struct DXFCPP_EXPORT String {
    inline static const std::string EMPTY{};
};

DXFCPP_EXPORT std::string toString(bool b) noexcept;

DXFCPP_EXPORT std::string toString(const char *chars) noexcept;

DXFCPP_EXPORT std::string toString(std::thread::id theadId);

DXFCPP_EXPORT std::string toString(void *ptr);

DXFCPP_EXPORT std::string toString(double d);

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

/**
 * Tries to convert UTF16 char to ASCII.
 *
 * @param in The UTF16 char
 * @return ASCII char
 */
DXFCPP_EXPORT char utf16to8(std::int16_t in) noexcept;

/**
 * Converts UTF16 char to UTF8 string
 *
 * @param in The UTF16 char
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string utf16toUtf8String(std::int16_t in) noexcept;

/**
 * Converts UTF16 string to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string utf16toUtf8String(const std::u16string &in) noexcept;

/**
 * Converts vector of UTF16 chars to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string utf16toUtf8String(const std::vector<std::int16_t> &in) noexcept;

DXFCPP_EXPORT std::int16_t utf8to16(char in) noexcept;

DXFCPP_EXPORT std::string formatTimeStamp(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithTimeZone(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithMillis(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp);

DXFCPP_EXPORT char *createCString(const std::string &s) noexcept;

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

template <typename It> std::string elementsToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + "'" + toStringAny(*it) + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

DXFCPP_EXPORT std::string encodeChar(std::int16_t c);

inline std::string encodeChar(char c) {
    return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c)));
}

namespace detail {

class IsIEqual {
    std::locale locale_;

  public:
    explicit IsIEqual(const std::locale &locale = std::locale()) : locale_{locale} {
    }

    template <typename T, typename U> bool operator()(const T &t, const U &u) const {
        return std::tolower<T>(t, locale_) == std::tolower<U>(u, locale_);
    }
};

} // namespace detail

template <typename Range1, typename Range2, typename Predicate>
inline bool equals(const Range1 &first, const Range2 &second, Predicate cmp) {
    auto firstIt = std::begin(first);
    auto secondIt = std::begin(second);

    for (; firstIt != std::end(first) && secondIt != std::end(second); ++firstIt, secondIt++) {
        if (!cmp(*firstIt, *secondIt)) {
            return false;
        }
    }

    return (secondIt == std::end(second)) && (firstIt == std::end(first));
}

template <typename Range1, typename Range2>
inline bool iEquals(const Range1 &first, const Range2 &second, const std::locale &locale = std::locale()) {
    return equals(first, second, detail::IsIEqual(locale));
}

} // namespace dxfcpp