// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

namespace {

using Arguments = std::vector<std::string>;

} // namespace

TEST_CASE("parseIsolateArguments() gives no arguments for an empty or whitespace-only text") {
    for (const char *text : {"", "   ", "\t\n\v\f\r", " \t \n "}) {
        const auto result = parseIsolateArguments(text);

        CHECK(result.ok());
        CHECK(result.error.empty());
        CHECK(result.arguments.empty());
    }
}

TEST_CASE("parseIsolateArguments() splits by the ASCII whitespaces") {
    auto result = parseIsolateArguments("-Xmx64m  -Dp=1");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Xmx64m", "-Dp=1"});

    result = parseIsolateArguments(" \t-a\n-b\v-c\f-d\r-e ");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-a", "-b", "-c", "-d", "-e"});
}

TEST_CASE("parseIsolateArguments() removes the quotes and keeps the whitespaces inside a group") {
    auto result = parseIsolateArguments("-Dp=\"a b\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=a b"});

    result = parseIsolateArguments("'-Dp=a b'");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=a b"});

    result = parseIsolateArguments("-Dp=\"a 'b' c\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=a 'b' c"});

    result = parseIsolateArguments("-Dp='a \"b\" c' -Dq=\"x\ty\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=a \"b\" c", "-Dq=x\ty"});
}

TEST_CASE("parseIsolateArguments() joins a quoted group with the rest of the argument") {
    auto result = parseIsolateArguments("a\"\"b");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"ab"});

    result = parseIsolateArguments("-Dname=\"a b\"c'd e'");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dname=a bcd e"});
}

TEST_CASE("parseIsolateArguments() gives an empty argument for an empty group") {
    auto result = parseIsolateArguments("\"\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{""});

    result = parseIsolateArguments("'' -a \"\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"", "-a", ""});
}

TEST_CASE("parseIsolateArguments() has no escapes: the backslash is an ordinary character") {
    auto result = parseIsolateArguments("-Dp=C:\\dir\\x");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=C:\\dir\\x"});

    result = parseIsolateArguments("\"a\\\" b");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"a\\", "b"});
}

TEST_CASE("parseIsolateArguments() keeps the bytes above 0x7F as is") {
    const std::string value = "-Dp=\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xE2\x82\xAC";

    auto result = parseIsolateArguments(value);

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=\xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82", "\xE2\x82\xAC"});

    result = parseIsolateArguments("-Dp=\"\xD0\xBF \xD1\x80\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=\xD0\xBF \xD1\x80"});
}

TEST_CASE("parseIsolateArguments() reports an unmatched quote with its offset and without the text") {
    auto result = parseIsolateArguments("-Dp=\"a");

    CHECK_FALSE(result.ok());
    CHECK(result.arguments.empty());
    CHECK(result.error.find("unmatched") != std::string::npos);
    CHECK(result.error.find('"') != std::string::npos);
    CHECK(result.error.find('4') != std::string::npos);
    CHECK(result.error.find("-Dp") == std::string::npos);

    result = parseIsolateArguments("-Xmx1g -Dsecret=top'secret");

    CHECK_FALSE(result.ok());
    CHECK(result.arguments.empty());
    CHECK(result.error.find("unmatched") != std::string::npos);
    CHECK(result.error.find('\'') != std::string::npos);
    CHECK(result.error.find("19") != std::string::npos);
    CHECK(result.error.find("secret") == std::string::npos);
    CHECK(result.error.find("Xmx") == std::string::npos);

    // The other kind of the quote does not close the group.
    result = parseIsolateArguments("'a\"");

    CHECK_FALSE(result.ok());
    CHECK(result.error.find("unmatched") != std::string::npos);
    CHECK(result.error.find('\'') != std::string::npos);
    CHECK(result.error.find('0') != std::string::npos);
}

TEST_CASE("parseIsolateArguments() does not support argument files, but accepts @ not in the first place") {
    auto result = parseIsolateArguments("@args.txt");

    CHECK_FALSE(result.ok());
    CHECK(result.arguments.empty());
    CHECK(result.error.find("@file") != std::string::npos);
    CHECK(result.error.find("not supported") != std::string::npos);
    CHECK(result.error.find("args.txt") == std::string::npos);

    result = parseIsolateArguments("-Xmx1g @f");

    CHECK_FALSE(result.ok());
    CHECK(result.arguments.empty());
    CHECK(result.error.find("@file") != std::string::npos);
    CHECK(result.error.find("Xmx1g") == std::string::npos);

    result = parseIsolateArguments("-Dp=@x");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"-Dp=@x"});

    // An @ inside the quotes is not the first character of the argument.
    result = parseIsolateArguments("\"@f\"");

    CHECK(result.ok());
    CHECK(result.arguments == Arguments{"@f"});
}
