// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <string>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

using namespace std::literals;
using namespace dxfcpp;

namespace {

// Base64("user:password")
constexpr auto USER_PASSWORD_BASE64 = "dXNlcjpwYXNzd29yZA==";

void checkBasicUserPassword(const AuthToken &token) {
    CHECK_EQ(token.getScheme(), AuthToken::BASIC_SCHEME);
    CHECK_EQ(token.getValue(), USER_PASSWORD_BASE64);
    CHECK_EQ(token.getUser(), "user");
    CHECK_EQ(token.getPassword(), "password");
    CHECK_EQ(token.getHttpAuthorization(), "Basic "s + USER_PASSWORD_BASE64);
}

} // namespace

TEST_SUITE("AuthToken") {

TEST_CASE("valueOf of a basic string") {
    const auto token = AuthToken::valueOf("Basic "s + USER_PASSWORD_BASE64);

    checkBasicUserPassword(token);
}

TEST_CASE("valueOf of a bearer string") {
    const auto token = AuthToken::valueOf("Bearer my-access-token");

    CHECK_EQ(token.getScheme(), AuthToken::BEARER_SCHEME);
    CHECK_EQ(token.getValue(), "my-access-token");
    CHECK_EQ(token.getHttpAuthorization(), "Bearer my-access-token");
    CHECK_EQ(token.getUser(), String::NUL);
    CHECK_EQ(token.getPassword(), String::NUL);
}

TEST_CASE("valueOf of a malformed string throws") {
    CHECK_THROWS_AS(AuthToken::valueOf("no-space-between-scheme-and-value"), JavaException);
}

TEST_CASE("createBasicToken from user:password") {
    const auto token = AuthToken::createBasicToken("user:password");

    checkBasicUserPassword(token);
    CHECK(token == AuthToken::valueOf("Basic "s + USER_PASSWORD_BASE64));
}

TEST_CASE("createBasicToken from a malformed user:password throws") {
    CHECK_THROWS_AS(AuthToken::createBasicToken("no-colon"), JavaException);
}

TEST_CASE("createBasicToken from user and password") {
    const auto token = AuthToken::createBasicToken("user", "password");

    checkBasicUserPassword(token);
    CHECK(token == AuthToken::createBasicToken("user:password"));
}

TEST_CASE("createBasicToken from an empty user and password") {
    const auto token = AuthToken::createBasicToken("", "");

    CHECK_EQ(token.getScheme(), AuthToken::BASIC_SCHEME);
    CHECK_EQ(token.getUser(), "");
    CHECK_EQ(token.getPassword(), "");
}

TEST_CASE("createBasicTokenOrNull") {
    const auto token = AuthToken::createBasicTokenOrNull("user", "password");

    checkBasicUserPassword(token);
    CHECK(token == AuthToken::createBasicToken("user", "password"));

    const auto nullToken = AuthToken::createBasicTokenOrNull("", "");

    CHECK(nullToken == AuthToken::NULL_TOKEN);
    CHECK_EQ(nullToken.getScheme(), String::NUL);
    CHECK_EQ(nullToken.getValue(), String::NUL);
    CHECK_EQ(nullToken.getUser(), String::NUL);
    CHECK_EQ(nullToken.getPassword(), String::NUL);
    CHECK_EQ(nullToken.getHttpAuthorization(), String::NUL);
}

TEST_CASE("createBearerToken") {
    const auto token = AuthToken::createBearerToken("my-access-token");

    CHECK_EQ(token.getScheme(), AuthToken::BEARER_SCHEME);
    CHECK_EQ(token.getValue(), "my-access-token");
    CHECK_EQ(token.getHttpAuthorization(), "Bearer my-access-token");
    CHECK_EQ(token.getUser(), String::NUL);
    CHECK_EQ(token.getPassword(), String::NUL);
    CHECK(token == AuthToken::valueOf("Bearer my-access-token"));
}

TEST_CASE("createBearerToken from an empty token throws") {
    CHECK_THROWS_AS(AuthToken::createBearerToken(""), JavaException);
}

TEST_CASE("createBearerTokenOrNull") {
    const auto token = AuthToken::createBearerTokenOrNull("my-access-token");

    CHECK_EQ(token.getScheme(), AuthToken::BEARER_SCHEME);
    CHECK_EQ(token.getValue(), "my-access-token");
    CHECK(token == AuthToken::createBearerToken("my-access-token"));

    const auto nullToken = AuthToken::createBearerTokenOrNull("");

    CHECK(nullToken == AuthToken::NULL_TOKEN);
    CHECK_EQ(nullToken.getScheme(), String::NUL);
    CHECK_EQ(nullToken.getValue(), String::NUL);
    CHECK_EQ(nullToken.getHttpAuthorization(), String::NUL);
}

TEST_CASE("createCustomToken") {
    const auto token = AuthToken::createCustomToken("Custom", "custom-value");

    CHECK_EQ(token.getScheme(), "Custom");
    CHECK_EQ(token.getValue(), "custom-value");
    CHECK_EQ(token.getHttpAuthorization(), "Custom custom-value");
    CHECK_EQ(token.getUser(), String::NUL);
    CHECK_EQ(token.getPassword(), String::NUL);
}

TEST_CASE("createCustomToken from an empty scheme or value throws") {
    CHECK_THROWS_AS(AuthToken::createCustomToken("", "custom-value"), JavaException);
    CHECK_THROWS_AS(AuthToken::createCustomToken("Custom", ""), JavaException);
}

TEST_CASE("A token string with invalid UTF-8 throws without its bytes") {
    CHECK_THROWS_WITH_AS(AuthToken::createBasicToken("user", "pass\xC3\x28"),
                         "AuthToken::createBasicToken(password): invalid UTF-8 at byte 4", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(AuthToken::createBasicToken("us\xFF:password"),
                         "AuthToken::createBasicToken(userPassword): invalid UTF-8 at byte 2",
                         InvalidArgumentException);
    CHECK_THROWS_WITH_AS(AuthToken::createBearerToken("\xED\xA0\x80"),
                         "AuthToken::createBearerToken(token): invalid UTF-8 at byte 0", InvalidArgumentException);
    CHECK_THROWS_WITH_AS(AuthToken::createCustomToken("Custom", "v\xC0\xAF"),
                         "AuthToken::createCustomToken(value): invalid UTF-8 at byte 1", InvalidArgumentException);
}

TEST_CASE("The scheme of a custom token is not a secret") {
    CHECK_THROWS_WITH_AS(AuthToken::createCustomToken("Cus\xFF", "value"),
                         "AuthToken::createCustomToken(scheme): invalid UTF-8 at byte 3: 0xFF",
                         InvalidArgumentException);
}

} // TEST_SUITE
