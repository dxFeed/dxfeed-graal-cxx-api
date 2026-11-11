// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../src/internal/utils/StringUtils.hpp"
#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>
#include <string_view>

using namespace dxfcpp;

namespace {

// The message of the exception that checkUtf8 throws, or an empty string when the string is valid.
std::string checkMessage(std::string_view s, const StringLabel &label = "Foo::bar(value)") {
    try {
        checkUtf8(s, label);
    } catch (const InvalidArgumentException &e) {
        return e.what();
    }

    return {};
}

} // namespace

TEST_CASE("checkUtf8: valid strings pass") {
    CHECK(checkMessage("").empty());
    CHECK(checkMessage("ASCII text 0123").empty());
    CHECK(checkMessage("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82").empty()); // Привет
    CHECK(checkMessage("\xF0\x9F\x98\x80").empty());                                 // U+1F600
    CHECK(checkMessage("\xF4\x8F\xBF\xBF").empty());                                 // U+10FFFF
    CHECK(checkMessage(std::string_view{"a\0b", 3}).empty());
}

TEST_CASE("checkUtf8: a stray byte") {
    CHECK(checkMessage("ab\xFF") == "Foo::bar(value): invalid UTF-8 at byte 2: 0xFF");
    CHECK(checkMessage("\x80z") == "Foo::bar(value): invalid UTF-8 at byte 0: 0x80");
}

TEST_CASE("checkUtf8: a broken sequence names the lead byte and the byte that broke it") {
    CHECK(checkMessage("demo:7300 a\xC3\x28") ==
          "Foo::bar(value): invalid UTF-8 at byte 11: 0xC3 0x28");
    CHECK(checkMessage("0123456789ab\xC3\x28", "DXEndpoint::connect(address)") ==
          "DXEndpoint::connect(address): invalid UTF-8 at byte 12: 0xC3 0x28");
}

TEST_CASE("checkUtf8: a truncated sequence") {
    CHECK(checkMessage("x\xE2\x82") == "Foo::bar(value): invalid UTF-8 at byte 1: 0xE2 0x82");
    CHECK(checkMessage("\xF0\x9F\x98") == "Foo::bar(value): invalid UTF-8 at byte 0: 0xF0 0x9F 0x98");
}

TEST_CASE("checkUtf8: an overlong form") {
    CHECK(checkMessage("a\xC0\xAF") == "Foo::bar(value): invalid UTF-8 at byte 1: 0xC0 0xAF");
    CHECK(checkMessage("\xE0\x80\xAF") == "Foo::bar(value): invalid UTF-8 at byte 0: 0xE0 0x80 0xAF");
}

TEST_CASE("checkUtf8: an encoded surrogate") {
    CHECK(checkMessage("\xED\xA0\x80") == "Foo::bar(value): invalid UTF-8 at byte 0: 0xED 0xA0 0x80");
}

TEST_CASE("checkUtf8: a value above U+10FFFF") {
    CHECK(checkMessage("\xF4\x90\x80\x80") == "Foo::bar(value): invalid UTF-8 at byte 0: 0xF4 0x90 0x80 0x80");
    CHECK(checkMessage("\xF8\x88\x80\x80\x80") == "Foo::bar(value): invalid UTF-8 at byte 0: 0xF8");
}

TEST_CASE("checkUtf8: a secret is named without its bytes") {
    CHECK(checkMessage("p\xC3\x28", StringLabel::secretOf("DXEndpoint::password(password)")) ==
          "DXEndpoint::password(password): invalid UTF-8 at byte 1");
}

TEST_CASE("checkUtf8: an event field is named by the type and the field") {
    CHECK(checkMessage("a\xFF", "TimeAndSale::exchangeSaleConditions") ==
          "TimeAndSale::exchangeSaleConditions: invalid UTF-8 at byte 1: 0xFF");
}
