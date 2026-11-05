// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "../InternalExport.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/detail/StringLike.hpp"
#include "../../../include/dxfeed_graal_cpp_api/util/StringUtils.hpp"

#include <cstdint>
#include <iterator>
#include <locale>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(_LIBCPP_VERSION)
#    include <charconv>
#endif

DXFCPP_BEGIN_NAMESPACE

DXFCPP_INTERNAL_EXPORT std::string toString(bool b) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string toString(const char *chars) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::optional<std::string>
toStringOpt(const char *chars) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string toString(std::thread::id theadId); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string toString(void *ptr); // NOLINT(*-redundant-declaration)

// template <typename T> std::string toStringAny(T &&t);

template <typename T, typename U> std::string toString(const std::pair<T, U> &p) {
    return "{" + toStringAny(p.first) + ", " + toStringAny(p.second) + "}";
}

#if __cpp_concepts
template <typename T> std::string toStringAny(T &&t) {
    if constexpr (requires { t.toString(); }) {
        return t.toString();
    } else if constexpr (requires { t->toString(); }) {
        return t->toString();
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
#else
namespace to_string_any {
namespace detail {
template <typename T, typename = void> struct HasToStringMember : std::false_type {};

template <typename T> struct HasToStringMember<T, decltype(std::declval<T>().toString(), void())> : std::true_type {};

template <typename T, typename = void> struct HasToStringMemberByPointer : std::false_type {};

template <typename T>
struct HasToStringMemberByPointer<T, decltype(std::declval<T>()->toString(), void())> : std::true_type {};

template <typename T, typename = void> struct HasFreeToString : std::false_type {};

template <typename T> struct HasFreeToString<T, decltype(toString(std::declval<T>()), void())> : std::true_type {};

template <typename T, typename = void> struct HasStdToString : std::false_type {};

template <typename T> struct HasStdToString<T, decltype(std::to_string(std::declval<T>()), void())> : std::true_type {};

template <typename T, typename = void> struct IsConvertibleToString : std::false_type {};

template <typename T>
struct IsConvertibleToString<T, typename std::enable_if<std::is_convertible<T, std::string>::value>::type>
    : std::true_type {};

} // namespace detail
} // namespace to_string_any

template <typename T>
typename std::enable_if<to_string_any::detail::HasToStringMember<T>::value, std::string>::type toStringAny(T &&t) {
    return t.toString();
}

template <typename T>
typename std::enable_if<to_string_any::detail::HasToStringMemberByPointer<T>::value &&
                            !to_string_any::detail::HasToStringMember<T>::value,
                        std::string>::type
toStringAny(T &&t) {
    return t->toString();
}

template <typename T>
typename std::enable_if<to_string_any::detail::HasFreeToString<T>::value &&
                            !to_string_any::detail::HasToStringMember<T>::value &&
                            !to_string_any::detail::HasToStringMemberByPointer<T>::value,
                        std::string>::type
toStringAny(T &&t) {
    return toString(t);
}

template <typename T>
typename std::enable_if<to_string_any::detail::HasStdToString<T>::value &&
                            !to_string_any::detail::HasToStringMember<T>::value &&
                            !to_string_any::detail::HasToStringMemberByPointer<T>::value &&
                            !to_string_any::detail::HasFreeToString<T>::value,
                        std::string>::type
toStringAny(T &&t) {
    return std::to_string(t);
}

template <typename T>
typename std::enable_if<
    to_string_any::detail::IsConvertibleToString<T>::value && !to_string_any::detail::HasToStringMember<T>::value &&
        !to_string_any::detail::HasToStringMemberByPointer<T>::value &&
        !to_string_any::detail::HasFreeToString<T>::value && !to_string_any::detail::HasStdToString<T>::value,
    std::string>::type
toStringAny(T &&t) {
    return std::string(t);
}

// Fallback case
template <typename T>
typename std::enable_if<!to_string_any::detail::HasToStringMember<T>::value &&
                            !to_string_any::detail::HasToStringMemberByPointer<T>::value &&
                            !to_string_any::detail::HasFreeToString<T>::value &&
                            !to_string_any::detail::HasStdToString<T>::value &&
                            !to_string_any::detail::IsConvertibleToString<T>::value,
                        std::string>::type
toStringAny(T &&) {
    return "unknown";
}
#endif

/**
 * Converts a UTF-16 code unit up to U+00FF to the char of the same Latin-1 code, as Java casts a char to a byte.
 *
 * @param in The UTF-16 code unit.
 * @return The char, or 0 if the code unit is above U+00FF.
 */
DXFCPP_INTERNAL_EXPORT char utf16to8(std::int16_t in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts UTF16 char to UTF8 string
 *
 * @param in The UTF16 char
 * @return UTF8 string
 */
DXFCPP_INTERNAL_EXPORT std::string utf16toUtf8String(std::int16_t in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts UTF16 string to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_INTERNAL_EXPORT std::string
utf16toUtf8String(const std::u16string &in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts vector of UTF16 chars to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_INTERNAL_EXPORT std::string
utf16toUtf8String(const std::vector<std::int16_t> &in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts a char to the UTF-16 code unit of the same Latin-1 code (0x00-0xFF), as Java stores a char code.
 *
 * @param in The char.
 * @return The UTF-16 code unit.
 */
DXFCPP_INTERNAL_EXPORT std::int16_t utf8to16(char in) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string formatTimeStamp(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string
formatTimeStampWithTimeZone(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string formatTimeStampFast(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string formatTimeStampWithMillis(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::string
formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT char *createCString(const StringLike &s); // NOLINT(*-redundant-declaration)

template <typename S> char *createCString(const std::optional<S> &s) {
    if (!s) {
        return nullptr;
    }

    return createCString(*s);
}

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; ++it) {
        result += "'" + it->getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> get().getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; ++it) {
        result += "'" + it->get().getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
std::string elementsToString(It begin, It end, const std::string &prefix = "[", const std::string &postfix = "[",
                             const std::string &separator = ", ") {
    std::string result{prefix};

    for (auto it = begin; it != end; ++it) {
        result += toStringAny(*it) + (std::next(it) == end ? "" : separator);
    }

    return result + postfix;
}

DXFCPP_INTERNAL_EXPORT std::string encodeChar(std::int16_t c); // NOLINT(*-redundant-declaration)

inline std::string encodeChar(char c) {
    return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c)));
}

namespace detail {

class DXFCPP_INTERNAL_EXPORT IsIEqual {
    std::locale locale_;

    public:
    explicit IsIEqual(const std::locale &locale = std::locale());

    template <typename T, typename U> bool operator()(const T &t, const U &u) const {
        return std::tolower<T>(t, locale_) == std::tolower<U>(u, locale_);
    }
};

} // namespace detail

template <typename Range1, typename Range2, typename Predicate>
bool equals(const Range1 &first, const Range2 &second, Predicate cmp) {
    auto firstIt = std::begin(first);
    auto secondIt = std::begin(second);

    for (; firstIt != std::end(first) && secondIt != std::end(second); ++firstIt, ++secondIt) {
        if (!cmp(*firstIt, *secondIt)) {
            return false;
        }
    }

    return (secondIt == std::end(second)) && (firstIt == std::end(first));
}

DXFCPP_INTERNAL_EXPORT inline bool iEquals(const StringLike &first, const StringLike &second) noexcept {
    const auto &locale = std::locale();

    return equals(first, second, detail::IsIEqual(locale));
}

DXFCPP_INTERNAL_EXPORT inline std::size_t icHash(const StringLike &s) noexcept {
    const auto &locale = std::locale();
    std::string result{};

    for (auto c : s) {
        result += std::tolower(c, locale);
    }

    return std::hash<std::string>()(result);
}

DXFCPP_INTERNAL_EXPORT std::string trimStr(const StringLike &s) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_INTERNAL_EXPORT std::vector<std::string> splitStr(const StringLike &s, char sep = ',') noexcept;

DXFCPP_INTERNAL_EXPORT std::string joinStr(const std::vector<StringLike> &v, const StringLike &sep = ", ") noexcept;

DXFCPP_INTERNAL_EXPORT bool toBool(const StringLike &s) noexcept; // NOLINT(*-redundant-declaration)

/**
 * The result of parseIsolateArguments(): the arguments, or the reason why the text cannot be parsed.
 */
struct DXFCPP_INTERNAL_EXPORT IsolateArgumentsParseResult {
    /// The arguments. Empty if the text has no arguments or if there is an error.
    std::vector<std::string> arguments{};

    /// The reason of the error. Empty on success. Never contains the text that was parsed.
    std::string error{};

    /// @return `true` if the text was parsed without an error.
    bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * Splits the text into the arguments of the isolate by the rules of the `JDK_JAVA_OPTIONS` environment variable.
 *
 * - The arguments are separated by the ASCII whitespaces (space, `\t`, `\n`, `\v`, `\f`, `\r`), regardless of the locale.
 * - A group in `'...'` or `"..."` keeps the whitespaces, the quotes are removed. A quote in the middle of an argument
 * joins the group with the rest of the argument: `-Dname="a b"` gives `-Dname=a b`, `a""b` gives `ab`. The quote of the
 * other kind inside a group is an ordinary character. `""` alone gives one empty argument.
 * - There are no escape sequences: the backslash is an ordinary character. The bytes above 0x7F are kept as is.
 * - An unmatched quote is an error. The message has the quote and its 0-based byte offset.
 * - An argument that starts with `@` outside the quotes (an argument file) is not supported and is an error.
 *
 * The error message never contains the text, as it can have secrets.
 *
 * @param text The text to split, e.g. the value of the `DXFCPP_ISOLATE_ARGUMENTS` environment variable.
 * @return The arguments, or the reason of the error.
 */
DXFCPP_INTERNAL_EXPORT IsolateArgumentsParseResult parseIsolateArguments(std::string_view text);

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
