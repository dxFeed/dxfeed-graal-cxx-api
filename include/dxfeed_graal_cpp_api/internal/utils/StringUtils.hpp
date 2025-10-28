// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstddef>
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
    std::string owned_;
    std::string_view view_;

    StringLike() = default;

    StringLike(const char *s) : owned_(s ? s : ""), view_(owned_) {
    }

    StringLike(std::string_view sv) : view_(sv) {
    }

    StringLike(const std::string &s) : owned_(s), view_(owned_) {
    }

    StringLike(std::string &&s) noexcept : owned_(std::move(s)), view_(owned_) {
    }

    template <std::size_t N> StringLike(const char (&arr)[N]) : view_(arr, N - 1) {
    }

    operator std::string_view() const noexcept {
        return view_;
    }
    operator std::string() const {
        return std::string(view_);
    }

    const char *data() const noexcept {
        return view_.data();
    }
    const char *c_str() const {
        return owned_.empty() ? std::string(view_).c_str() : owned_.c_str();
    }

    auto begin() const noexcept {
        return view_.begin();
    }
    auto end() const noexcept {
        return view_.end();
    }
    auto cbegin() const noexcept {
        return view_.cbegin();
    }
    auto cend() const noexcept {
        return view_.cend();
    }

    bool empty() const noexcept {
        return view_.empty();
    }
    std::size_t size() const noexcept {
        return view_.size();
    }
    std::size_t length() const noexcept {
        return view_.size();
    }

    bool ends_with(const StringLike &other) const noexcept {
#if __cpp_lib_ends_with >= 201907L
        return view_.ends_with(other.view_);
#else
        auto sv = other.view_;
        return sv.size() <= view_.size() && view_.compare(view_.size() - sv.size(), sv.size(), sv) == 0;
#endif
    }

    std::string substr(std::size_t pos = 0, std::size_t count = std::string::npos) const {
        auto sv2 = view_.substr(pos, count);
        return std::string(sv2);
    }

    bool operator==(const StringLike &other) const noexcept {
        return view_ == other.view_;
    }

    friend std::string operator+(const StringLike &a, const StringLike &b) {
        std::string result;
        result.reserve(a.size() + b.size());
        result.append(a.view_);
        result.append(b.view_);
        return result;
    }

    explicit operator double() const {
        double result{};
#if defined(_LIBCPP_VERSION)
        result = std::stod(std::string(view_));
#else
        if (auto [ptr, ec] = std::from_chars(view_.data(), view_.data() + view_.size(), result); ec != std::errc{})
            throw std::invalid_argument("StringLike: cannot convert to double");
#endif
        return result;
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

    std::size_t operator()(const char *str) const {
        return HashType{}(str);
    }
    std::size_t operator()(std::string_view sv) const {
        return HashType{}(sv);
    }
    std::size_t operator()(const std::string &str) const {
        return HashType{}(str);
    }
    std::size_t operator()(const StringLike &s) const {
        return HashType{}(s);
    }
};

struct DXFCPP_EXPORT String {
    inline static const std::string EMPTY{};
    inline static const std::string NUL{"<null>"};
};

DXFCPP_EXPORT std::string toString(bool b) noexcept;

DXFCPP_EXPORT std::string toString(const char *chars) noexcept;

DXFCPP_EXPORT std::optional<std::string> toStringOpt(const char *chars) noexcept;

DXFCPP_EXPORT std::string toString(std::thread::id theadId);

DXFCPP_EXPORT std::string toString(void *ptr);

DXFCPP_EXPORT std::string toString(double d);

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

/**
 * Tries to convert ASCII (part of UTF8) char to UTF16 char.
 *
 * @param in The ASCII char
 * @return The UTF16 char
 */
DXFCPP_EXPORT std::int16_t utf8to16(char in) noexcept;

DXFCPP_EXPORT std::string formatTimeStamp(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithTimeZone(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampFast(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithMillis(std::int64_t timestamp);

DXFCPP_EXPORT std::string formatTimeStampWithMillisWithTimeZone(std::int64_t timestamp);

DXFCPP_EXPORT char *createCString(const StringLike &s);

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

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + "'" + it->getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
    requires requires { std::is_same_v<std::decay_t<decltype(It {} -> get().getName())>, std::string>; }
std::string namesToString(It begin, It end) {
    std::string result{"["};

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + "'" + it->get().getName() + "'" + (std::next(it) == end ? "" : ", ");
    }

    return result + "]";
}

template <typename It>
std::string elementsToString(It begin, It end, const std::string &prefix = "[", const std::string &postfix = "[",
                             const std::string &separator = ", ") {
    std::string result{prefix};

    for (auto it = begin; it != end; it++) {
        result += String::EMPTY + toStringAny(*it) + (std::next(it) == end ? "" : separator);
    }

    return result + postfix;
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

    for (; firstIt != std::end(first) && secondIt != std::end(second); ++firstIt, ++secondIt) {
        if (!cmp(*firstIt, *secondIt)) {
            return false;
        }
    }

    return (secondIt == std::end(second)) && (firstIt == std::end(first));
}

DXFCPP_EXPORT inline bool iEquals(const StringLike &first, const StringLike &second) noexcept {
    const std::locale &locale = std::locale();

    return equals(first, second, detail::IsIEqual(locale));
}

DXFCPP_EXPORT inline std::size_t icHash(const StringLike &s) noexcept {
    const std::locale &locale = std::locale();
    std::string result{};

    for (auto c : s) {
        result += std::tolower(c, locale);
    }

    return std::hash<std::string>()(result);
}

DXFCPP_EXPORT std::string trimStr(const StringLike &s) noexcept;

DXFCPP_EXPORT std::vector<std::string> splitStr(const StringLike &s, char sep = ',') noexcept;

DXFCPP_EXPORT std::string joinStr(const std::vector<StringLike> &v, const StringLike &sep = ", ") noexcept;

DXFCPP_EXPORT bool toBool(const StringLike &s) noexcept;

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()