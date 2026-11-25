// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace dxfcpp;
using namespace std::literals;

namespace {

// KTD1: own layout of a pointer and a length.
static_assert(std::is_trivially_copyable_v<StringParam>);
static_assert(std::is_standard_layout_v<StringParam>);
static_assert(sizeof(StringParam) == 2 * sizeof(void *));

// Accepted forms.
static_assert(std::is_convertible_v<const char *, StringParam>);
static_assert(std::is_convertible_v<char *, StringParam>);
static_assert(std::is_convertible_v<std::nullptr_t, StringParam>);
static_assert(std::is_convertible_v<const char (&)[4], StringParam>);
static_assert(std::is_convertible_v<char (&)[8], StringParam>);
static_assert(std::is_convertible_v<std::string, StringParam>);
static_assert(std::is_convertible_v<const std::string &, StringParam>);
static_assert(std::is_convertible_v<std::string_view, StringParam>);
static_assert(std::is_convertible_v<std::vector<char>, StringParam>);
static_assert(std::is_convertible_v<const std::vector<char> &, StringParam>);
static_assert(std::is_convertible_v<std::array<char, 4>, StringParam>);
static_assert(std::is_convertible_v<std::span<const char>, StringParam>);
static_assert(std::is_convertible_v<std::span<char>, StringParam>);
static_assert(std::is_convertible_v<StringParam, StringParam>);

// R4: what is not a string is not accepted.
struct OnlyUserOperator {
    operator std::string() const; // NOLINT
};

static_assert(!std::is_convertible_v<int, StringParam>);
static_assert(!std::is_convertible_v<std::int64_t, StringParam>);
static_assert(!std::is_convertible_v<bool, StringParam>);
static_assert(!std::is_convertible_v<double, StringParam>);
static_assert(!std::is_convertible_v<std::optional<std::string>, StringParam>);
static_assert(!std::is_convertible_v<std::wstring, StringParam>);
static_assert(!std::is_convertible_v<const wchar_t *, StringParam>);
static_assert(!std::is_convertible_v<const char16_t *, StringParam>);
static_assert(!std::is_convertible_v<std::vector<int>, StringParam>);
// Only the implicit conversion matters for a parameter: an explicit construction may use the user's operator once.
static_assert(!std::is_convertible_v<OnlyUserOperator, StringParam>);
static_assert(!std::is_convertible_v<const char8_t *, StringParam>);
static_assert(!std::is_convertible_v<std::u8string, StringParam>);
static_assert(!std::is_constructible_v<StringParam, int>);
static_assert(!std::is_constructible_v<StringParam, bool>);
static_assert(!std::is_constructible_v<StringParam, std::optional<std::string>>);
static_assert(!std::is_constructible_v<StringParam, std::wstring>);
static_assert(!std::is_constructible_v<StringParam, const char16_t *>);
static_assert(!std::is_constructible_v<StringParam, std::vector<int>>);
static_assert(!std::is_constructible_v<StringParam, const char8_t *>);
static_assert(!std::is_constructible_v<StringParam, std::u8string>);

// KTD2: no implicit conversions from StringParam, so it is not a string symbol.
static_assert(!std::is_convertible_v<StringParam, std::string>);
static_assert(!std::is_convertible_v<StringParam, std::string_view>);
static_assert(!std::is_convertible_v<StringParam, SymbolWrapper>);
static_assert(!std::is_constructible_v<SymbolWrapper, StringParam>);
static_assert(!ConvertibleToStringSymbol<StringParam>);

static_assert(std::is_nothrow_copy_constructible_v<StringParam>);
static_assert(noexcept(StringParam{std::declval<const std::string &>()}));
static_assert(noexcept(StringParam{"abc"}));

void expectEq(const StringParam &p, std::string_view expected) {
    CHECK(p.size() == expected.size());
    CHECK(p.empty() == expected.empty());
    CHECK(p.view() == expected);
    CHECK(std::string_view{p.data(), p.size()} == expected);
}

// KTD4 subset: local overload sets.
std::string pick(StringParam) {
    return "string";
}

std::string pick(std::int32_t) {
    return "int";
}

struct Thing {};

std::string pickPtr(StringParam) {
    return "string";
}

std::string pickPtr(const std::shared_ptr<Thing> &) {
    return "ptr";
}

std::size_t sizeOf(StringParam p) {
    return p.size();
}

} // namespace

TEST_CASE("StringParam: every form keeps the content") {
    const char *cstr = "hello";
    char mutableStr[] = "hello";
    char *pmutable = mutableStr;
    const std::string str = "hello";
    const std::string_view sv = "hello"sv;
    const std::vector<char> vec{'h', 'e', 'l', 'l', 'o'};
    const std::array<char, 5> arr{'h', 'e', 'l', 'l', 'o'};
    const std::span<const char> spanConst{vec};
    std::vector<char> mutVec{'h', 'e', 'l', 'l', 'o'};
    const std::span<char> spanMut{mutVec};

    expectEq(cstr, "hello");
    expectEq(pmutable, "hello");
    expectEq("hello", "hello");
    expectEq(mutableStr, "hello");
    expectEq(str, "hello");
    expectEq(sv, "hello");
    expectEq(vec, "hello");
    expectEq(arr, "hello");
    expectEq(spanConst, "hello");
    expectEq(spanMut, "hello");
    expectEq(std::string{"hello"}, "hello");
    expectEq(std::vector<char>{'h', 'i'}, "hi");
    expectEq(StringParam{StringParam{"x"}}, "x");
}

TEST_CASE("StringParam: embedded NUL in a string, a view and a range keeps the full size") {
    const std::string str{"ab\0cd", 5};
    const std::string_view sv{"ab\0cd", 5};
    const std::vector<char> vec{'a', 'b', '\0', 'c', 'd'};

    expectEq(str, sv);
    CHECK(StringParam{str}.size() == 5);
    CHECK(StringParam{sv}.size() == 5);
    CHECK(StringParam{vec}.size() == 5);
}

TEST_CASE("StringParam: std::array keeps trailing NULs, unlike a char array") {
    const std::array<char, 4> arr{'a', 'b', '\0', '\0'};
    const char carr[4] = {'a', 'b', '\0', '\0'};

    CHECK(StringParam{arr}.size() == 4);
    CHECK(StringParam{carr}.size() == 2);
}

TEST_CASE("StringParam: null and empty") {
    const char *cnull = nullptr;
    char *pnull = nullptr;

    expectEq(cnull, "");
    expectEq(pnull, "");
    expectEq(nullptr, "");
    expectEq(StringParam{}, "");
    expectEq("", "");
    expectEq(std::string{}, "");
    expectEq(std::string_view{}, "");
    expectEq(std::vector<char>{}, "");
    expectEq(std::span<const char>{}, "");
    CHECK(StringParam{cnull}.data() != nullptr);
    CHECK(StringParam{std::string_view{}}.data() != nullptr);
    CHECK(StringParam{std::vector<char>{}}.data() != nullptr);
}

TEST_CASE("StringParam: char arrays end at the first NUL within the array, never past it") {
    char buf[8] = "ab";
    char full[3] = {'a', 'b', 'c'};
    const char cbuf[8] = "ab";
    const char cfull[3] = {'a', 'b', 'c'};
    char allNul[4] = {};
    char single[1] = {'x'};

    expectEq(buf, "ab");
    expectEq(full, "abc");
    expectEq(cbuf, "ab");
    expectEq(cfull, "abc");
    expectEq("abc", "abc");
    expectEq(allNul, "");
    expectEq(single, "x");
    CHECK(StringParam{"abc"}.size() == 3);
    CHECK(StringParam{buf}.data() == static_cast<const char *>(buf));
}

TEST_CASE("StringParam: overload resolution") {
    CHECK(pick(0) == "int");
    CHECK(pick(std::int32_t{42}) == "int");
    CHECK(pick("0") == "string");
    CHECK(pick(std::string{"0"}) == "string");
    CHECK(pickPtr(static_cast<const char *>(nullptr)) == "string");
    CHECK(pickPtr("abc") == "string");
    CHECK(pickPtr(std::shared_ptr<Thing>{}) == "ptr");

    char buf[8] = "abc";
    const char *cstr = "abcd";

    CHECK(sizeOf("abc") == 3);
    CHECK(sizeOf(buf) == 3);
    CHECK(sizeOf(cstr) == 4);
    CHECK(sizeOf(std::string{"abcde"}) == 5);
    CHECK(sizeOf(std::vector<char>{'a', 'b'}) == 2);
}
