// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../src/isolated/SdkString.hpp"
#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using dxfcpp::isolated::SdkString;

TEST_CASE("SdkString: an empty string") {
    CHECK(std::strlen(SdkString{std::string_view{}}.c_str()) == 0);
    CHECK(std::strlen(SdkString{""}.c_str()) == 0);
    CHECK(std::strlen(SdkString{static_cast<const char *>(nullptr)}.c_str()) == 0);

    const std::string empty;

    CHECK(std::strlen(SdkString{empty}.c_str()) == 0);
}

TEST_CASE("SdkString: a view onto a part of an array is terminated after the view") {
    const char buffer[] = {'a', 'b', 'c', 'd', 'e'};
    const SdkString s{std::string_view{buffer + 1, 3}};

    CHECK(std::strlen(s.c_str()) == 3);
    CHECK(std::string{s.c_str()} == "bcd");
    CHECK(s.c_str()[3] == '\0');
    CHECK(s.c_str() != buffer + 1);
}

TEST_CASE("SdkString: an embedded NUL cuts the string for the SDK") {
    const std::string_view view{"ab\0cd", 5};
    const SdkString s{view};

    CHECK(std::strlen(s.c_str()) == 2);
    CHECK(std::string{s.c_str()} == "ab");
}

TEST_CASE("SdkString: exact-size buffers without NUL are not over-read") {
    const char full[3] = {'a', 'b', 'c'};

    CHECK(std::string{SdkString{std::string_view{full, 3}}.c_str()} == "abc");

    const std::vector<char> vec{'x', 'y', 'z'};

    CHECK(std::string{SdkString{std::string_view{vec.data(), vec.size()}}.c_str()} == "xyz");
}

TEST_CASE("SdkString: a literal and a const char * are copied") {
    CHECK(std::string{SdkString{"literal"}.c_str()} == "literal");

    const char *p = "pointer";
    const SdkString s{p};

    CHECK(std::string{s.c_str()} == "pointer");
    CHECK(s.c_str() != p);
}

TEST_CASE("SdkString: a std::string lvalue is borrowed, a temporary is owned") {
    const std::string str = "borrowed string, longer than the small buffer of a std::string";

    CHECK(SdkString{str}.c_str() == str.c_str());

    std::string mutableStr = "m";

    CHECK(SdkString{mutableStr}.c_str() == mutableStr.c_str());
    CHECK(std::string{SdkString{std::string{"temporary"}}.c_str()} == "temporary");
}
