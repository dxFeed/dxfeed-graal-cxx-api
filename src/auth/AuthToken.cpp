// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

const AuthToken AuthToken::NULL_TOKEN(JavaObjectHandle<AuthToken>(), true);

AuthToken AuthToken::valueOf(const StringLike &string) {
    return AuthToken(isolated::auth::IsolatedAuthToken::valueOf(string));
}

AuthToken AuthToken::createBasicToken(const StringLike &userPassword) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBasicToken(userPassword));
}

AuthToken AuthToken::createBasicToken(const StringLike &user, const StringLike &password) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBasicToken(user, password));
}

AuthToken AuthToken::createBasicTokenOrNull(const StringLike &user, const StringLike &password) {
    auto handle = isolated::auth::IsolatedAuthToken::createBasicTokenOrNull(user, password);

    if (!handle) {
        return AuthToken(JavaObjectHandle<AuthToken>(), true);
    }

    return AuthToken(std::move(handle));
}

AuthToken AuthToken::createBearerToken(const StringLike &token) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBearerToken(token));
}

AuthToken AuthToken::createBearerTokenOrNull(const StringLike &token) {
    auto handle = isolated::auth::IsolatedAuthToken::createBearerTokenOrNull(token);

    if (!handle) {
        return AuthToken(JavaObjectHandle<AuthToken>(), true);
    }

    return AuthToken(std::move(handle));
}

AuthToken AuthToken::createCustomToken(const StringLike &scheme, const StringLike &value) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createCustomToken(scheme, value));
}

std::string AuthToken::getHttpAuthorization() const {
    if (isNull_) {
        return String::NUL;
    }

    return isolated::auth::IsolatedAuthToken::getHttpAuthorization(handle_);
}

std::string AuthToken::getUser() const {
    if (isNull_) {
        return String::NUL;
    }

    return isolated::auth::IsolatedAuthToken::getUser(handle_);
}

std::string AuthToken::getPassword() const {
    if (isNull_) {
        return String::NUL;
    }

    return isolated::auth::IsolatedAuthToken::getPassword(handle_);
}

std::string AuthToken::getScheme() const {
    if (isNull_) {
        return String::NUL;
    }

    return isolated::auth::IsolatedAuthToken::getScheme(handle_);
}

std::string AuthToken::getValue() const {
    if (isNull_) {
        return String::NUL;
    }

    return isolated::auth::IsolatedAuthToken::getValue(handle_);
}

std::string AuthToken::toString() const {
    if (isNull_) {
        return "AuthToken{<null>}";
    }

    return JavaObject::toString(handle_.get());
}

std::size_t AuthToken::hashCode() const noexcept {
    if (isNull_) {
        return 0;
    }

    return JavaObject::hashCode(handle_.get());
}

bool AuthToken::operator==(const AuthToken &other) const {
    if (isNull_) {
        return other.isNull_;
    }

    if (other.isNull_) {
        return false;
    }

    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

AuthToken::AuthToken(JavaObjectHandle<AuthToken> &&handle, bool isNull) : handle_(std::move(handle)), isNull_(isNull) {
}

DXFCPP_END_NAMESPACE