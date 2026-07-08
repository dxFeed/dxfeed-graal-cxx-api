// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <iterator>
#include <locale>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#if !defined(_LIBCPP_VERSION)
#    include <charconv>
#endif

DXFCPP_BEGIN_NAMESPACE

/// A lightweight wrapper around strings or string-like inputs.
/// Stores either a view or an owned string but always exposes a valid view.
struct StringLike {
    mutable std::string owned_;
    mutable std::string_view view_;

    StringLike();

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(const char *s);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(std::string_view sv);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(const std::string &s);

    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringLike(std::string &&s) noexcept;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    template <std::size_t N> StringLike(const char (&arr)[N]) : view_(arr, N - 1) {
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::string_view() const noexcept;

    // ReSharper disable once CppNonExplicitConversionOperator
    operator std::string() const;

    const char *data() const noexcept;

    const char *c_str() const;

    std::string_view::const_iterator begin() const noexcept;

    std::string_view::const_iterator end() const noexcept;

    std::string_view::const_iterator cbegin() const noexcept;

    std::string_view::const_iterator cend() const noexcept;

    bool empty() const noexcept;

    std::size_t size() const noexcept;

    std::size_t length() const noexcept;

    bool ends_with(const StringLike &other) const noexcept;

    std::string substr(std::size_t pos = 0, std::size_t count = std::string::npos) const;

    bool operator==(const StringLike &other) const noexcept;

    friend std::string operator+(const StringLike &a, const StringLike &b) {
        std::string result;

        result.reserve(a.size() + b.size());
        result.append(a.view_);
        result.append(b.view_);

        return result;
    }

    explicit operator double() const;

    std::string toString() const noexcept;

    std::string_view toStringView() const noexcept;

    friend std::ostream &operator<<(std::ostream &os, const StringLike &sl) {
        return os << sl.view_;
    }
};

DXFCPP_END_NAMESPACE

template <> struct DXFCPP_EXPORT std::hash<dxfcpp::StringLike> {
    std::size_t operator()(const dxfcpp::StringLike &sl) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(sl));
    }
};

DXFCPP_BEGIN_NAMESPACE

/// Universal functional object that allows searching std::unordered_map for string-like keys.
struct StringHash {
    using HashType = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char *str) const;
    std::size_t operator()(std::string_view sv) const;
    std::size_t operator()(const std::string &str) const;
    std::size_t operator()(const StringLike &s) const;
};

/**
 * Common string constants used across the library.
 *
 * `EMPTY` represents an empty string value.
 * `NUL` is a textual placeholder for a missing or null string value
 * and is intended for logging, debugging, and diagnostic output.
 */
struct DXFCPP_EXPORT String {
    /// Empty string constant: `""`.
    inline static const std::string EMPTY{};

    /// Textual representation of a null / missing string: `"<null>"`.
    inline static const std::string NUL{"<null>"};
};

DXFCPP_EXPORT std::string toString(bool b) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string toString(const char *chars) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::optional<std::string> toStringOpt(const char *chars) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string toString(std::thread::id theadId); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string toString(void *ptr); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string toString(double d); // NOLINT(*-redundant-declaration)

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
 * Tries to convert UTF16 char to ASCII (part of UTF8) char.
 *
 * @param in The UTF16 char
 * @return ASCII char
 */
DXFCPP_EXPORT char utf16to8(std::int16_t in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts UTF16 char to UTF8 string
 *
 * @param in The UTF16 char
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string utf16toUtf8String(std::int16_t in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts UTF16 string to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string utf16toUtf8String(const std::u16string &in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Converts vector of UTF16 chars to UTF8 string
 *
 * @param in The UTF16 string
 * @return UTF8 string
 */
DXFCPP_EXPORT std::string
utf16toUtf8String(const std::vector<std::int16_t> &in) noexcept; // NOLINT(*-redundant-declaration)

/**
 * Tries to convert ASCII (part of UTF8) char to UTF16 char.
 *
 * @param in The ASCII char
 * @return The UTF16 char
 */
DXFCPP_EXPORT std::int16_t utf8to16(char in) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string formatTimeStamp(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string formatTimeStampWithTimeZone(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string formatTimeStampFast(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string formatTimeStampWithMillis(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::string
formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp); // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT char *createCString(const StringLike &s); // NOLINT(*-redundant-declaration)

template <typename S> char *createCString(const std::optional<S> &s) {
    if (!s) {
        return nullptr;
    }

    return createCString(s.value());
}

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; ++it) {
        result += String::EMPTY + "'" + it->getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> get().getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; ++it) {
        result += String::EMPTY + "'" + it->get().getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
std::string elementsToString(It begin, It end, const std::string &prefix = "[", const std::string &postfix = "[",
                             const std::string &separator = ", ") {
    std::string result{prefix};

    for (auto it = begin; it != end; ++it) {
        result += String::EMPTY + toStringAny(*it) + (std::next(it) == end ? "" : separator);
    }

    return result + postfix;
}

DXFCPP_EXPORT std::string encodeChar(std::int16_t c); // NOLINT(*-redundant-declaration)

inline std::string encodeChar(char c) {
    return encodeChar(static_cast<std::int16_t>(static_cast<unsigned char>(c)));
}

namespace detail {

class IsIEqual {
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

DXFCPP_EXPORT inline bool iEquals(const StringLike &first, const StringLike &second) noexcept {
    const auto &locale = std::locale();

    return equals(first, second, detail::IsIEqual(locale));
}

DXFCPP_EXPORT inline std::size_t icHash(const StringLike &s) noexcept {
    const auto &locale = std::locale();
    std::string result{};

    for (auto c : s) {
        result += std::tolower(c, locale);
    }

    return std::hash<std::string>()(result);
}

DXFCPP_EXPORT std::string trimStr(const StringLike &s) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_EXPORT std::vector<std::string> splitStr(const StringLike &s, char sep = ',') noexcept;

DXFCPP_EXPORT std::string joinStr(const std::vector<StringLike> &v, const StringLike &sep = ", ") noexcept;

DXFCPP_EXPORT bool toBool(const StringLike &s) noexcept; // NOLINT(*-redundant-declaration)

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()