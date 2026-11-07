// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The type declared here appears in public signatures, and its name is part of the API; this header's location is not:
// include dxfeed_graal_cpp_api/api.hpp.

#pragma once

#include "./Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

DXFCPP_BEGIN_NAMESPACE

struct StringParam;

/**
 * Describes a contiguous sized range of `char` other than a C array, `std::string`, `std::string_view` and
 * StringParam (for example `std::vector<char>`, `std::array<char, N>`, `std::span<const char>`).
 *
 * @tparam R The range type (possibly a reference)
 */
template <typename R>
concept StringParamRange =
    std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
    std::same_as<std::ranges::range_value_t<R>, char> && !std::is_array_v<std::remove_cvref_t<R>> &&
    !std::same_as<std::remove_cvref_t<R>, std::string> && !std::same_as<std::remove_cvref_t<R>, std::string_view> &&
    !std::same_as<std::remove_cvref_t<R>, StringParam>;

/**
 * Describes a pointer to characters that StringParam accepts: exactly `const char *`, `char *` or `std::nullptr_t`
 * (a C array is not a pointer here).
 *
 * @tparam P The argument type (possibly a reference)
 */
template <typename P>
concept StringParamPointer =
    std::same_as<std::remove_cvref_t<P>, const char *> || std::same_as<std::remove_cvref_t<P>, char *> ||
    std::same_as<std::remove_cvref_t<P>, std::nullptr_t>;

/**
 * A non-owning string parameter: a pointer to characters and their count.
 *
 * It is a parameter only: it must not be stored, returned or kept after the call it is passed to, because it does not
 * own the characters. A temporary (a `std::string`, a `std::vector<char>`) lives until the end of the full expression
 * of the call, which is enough for a parameter.
 *
 * The name `dxfcpp::StringParam` is part of the API: it is spelled in the signatures of the functions that take a
 * string, including the virtual functions that users override.
 *
 * Accepted: a literal, a `char` array, `const char *` and `char *`, `std::string`, `std::string_view`, and contiguous
 * ranges of `char` (`std::vector<char>`, `std::array<char, N>`, `std::span<const char>`, `std::span<char>`).
 * Not accepted: numbers, `bool`, `std::optional`, wide and UTF-8 strings, ranges of other types, and types that only
 * convert to `std::string` by their own operator.
 *
 * Rules of the length:
 * - A `char` array (a literal, a buffer) ends at the first NUL within the array, or at the array's end when there is
 *   none: `char buf[8] = "ab"` is "ab", `char full[3] = {'a', 'b', 'c'}` is "abc" (nothing is read past the array).
 * - A `std::array<char, N>` is a range, not a C array: it keeps its trailing NULs.
 * - A null pointer is an empty string.
 * - A `std::string`, a `std::string_view` and a range keep their whole size, including embedded NULs; the type never
 *   truncates. The boundary with the dxFeed Graal SDK reads the characters up to the first NUL.
 *
 * Passing a bare `nullptr` to a function that has another overload with a smart pointer parameter is ambiguous (for
 * example `Schedule::getInstance`): write `static_cast<const char *>(nullptr)`.
 *
 * `data()` is not guaranteed to be NUL-terminated. There is no implicit conversion to `std::string` or
 * `std::string_view`; use view().
 */
struct StringParam final {
    private:
    const char *data_;
    std::size_t size_;

    public:
    /// Creates an empty string.
    constexpr StringParam() noexcept : data_{""}, size_{0} {
    }

    /**
     * Creates a string from a `char` array (a literal or a buffer): the characters up to the first NUL within the
     * array, or the whole array when it has no NUL.
     *
     * @tparam N The array size
     * @param arr The array
     */
    template <std::size_t N>
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr StringParam(const char (&arr)[N]) noexcept : data_{arr}, size_{0} {
        while (size_ < N && arr[size_] != '\0') {
            ++size_;
        }
    }

    /**
     * Creates a string from a pointer to NUL-terminated characters. A null pointer is an empty string.
     *
     * @tparam P `const char *`, `char *` or `std::nullptr_t`
     * @param str The pointer
     */
    template <typename P>
        requires StringParamPointer<P>
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr StringParam(P &&str) noexcept : data_{""}, size_{0} {
        if constexpr (!std::same_as<std::remove_cvref_t<P>, std::nullptr_t>) {
            const char *p = str;

            if (p != nullptr) {
                data_ = p;
                size_ = std::char_traits<char>::length(p);
            }
        }
    }

    /**
     * Creates a string from a `std::string`, the whole of it.
     *
     * @param str The string
     */
    // ReSharper disable once CppNonExplicitConvertingConstructor
    StringParam(const std::string &str) noexcept : data_{str.data()}, size_{str.size()} {
    }

    /**
     * Creates a string from a `std::string_view`, the whole of it.
     *
     * @param sv The view
     */
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr StringParam(std::string_view sv) noexcept
        : data_{sv.data() != nullptr ? sv.data() : ""}, size_{sv.size()} {
    }

    /**
     * Creates a string from a contiguous range of `char`, the whole of it.
     *
     * @tparam R The range type
     * @param range The range
     */
    template <typename R>
        requires StringParamRange<R>
    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr StringParam(R &&range) noexcept
        : data_{std::ranges::size(range) == 0 ? "" : std::ranges::data(range)}, size_{std::ranges::size(range)} {
    }

    /// @return The characters; never null, not guaranteed to be NUL-terminated.
    constexpr const char *data() const noexcept {
        return data_;
    }

    /// @return The number of characters.
    constexpr std::size_t size() const noexcept {
        return size_;
    }

    /// @return `true` if there are no characters.
    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    /// @return A view of the characters.
    constexpr std::string_view view() const noexcept {
        return {data_, size_};
    }
};

static_assert(std::is_trivially_copyable_v<StringParam>);
static_assert(std::is_standard_layout_v<StringParam>);
static_assert(sizeof(StringParam) == 2 * sizeof(void *));

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
