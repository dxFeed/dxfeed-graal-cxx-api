// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <optional>
#include <string>

using namespace dxfcpp;

namespace {

// Takes the string made by createCString() and frees it.
std::optional<std::string> take(const char *s) {
    if (s == nullptr) {
        return std::nullopt;
    }

    std::string result{s};

    delete[] s;

    return result;
}

} // namespace

TEST_CASE("createCString() copies the text \"<null>\" as text, only an empty optional is null") {
    CHECK(take(createCString(StringLike{"<null>"})) == "<null>");
    CHECK(take(createCString(StringLike{""})) == "");
    CHECK(take(createCString(std::optional<std::string>{"<null>"})) == "<null>");
    CHECK(take(createCString(std::optional<std::string>{})) == std::nullopt);
}

TEST_CASE("utf8to16() and utf16to8() convert the characters up to U+00FF as Latin-1, as Java converts char and byte") {
    CHECK(utf8to16('\0') == 0);
    CHECK(utf8to16('Q') == 'Q');
    CHECK(utf8to16('\x80') == 0x80);
    CHECK(utf8to16('\xE2') == 0xE2);
    CHECK(utf8to16('\xFF') == 0xFF);

    CHECK(utf16to8(0) == '\0');
    CHECK(utf16to8('Q') == 'Q');
    CHECK(utf16to8(0x80) == '\x80');
    CHECK(utf16to8(0xE2) == '\xE2');
    CHECK(utf16to8(0xFF) == '\xFF');
    CHECK(utf16to8(0x0100) == '\0');
    CHECK(utf16to8(0x20AC) == '\0');
    CHECK(utf16to8(static_cast<std::int16_t>(0xFFFF)) == '\0');
}
