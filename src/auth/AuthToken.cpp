// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/auth/AuthToken.hpp"

#include "../isolated/auth/IsolatedAuthToken.hpp"

#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

// Constant-initialized (constinit), so it can be used in the dynamic initialization of global variables of other
// translation units.
constinit const AuthToken AuthToken::NULL_TOKEN{};

AuthToken AuthToken::valueOf(StringParam string) {
    return AuthToken(isolated::auth::IsolatedAuthToken::valueOf(string.view()));
}

AuthToken AuthToken::createBasicToken(StringParam userPassword) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBasicToken(userPassword.view()));
}

AuthToken AuthToken::createBasicToken(StringParam user, StringParam password) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBasicToken(user.view(), password.view()));
}

AuthToken AuthToken::createBasicTokenOrNull(StringParam user, StringParam password) {
    auto handle = isolated::auth::IsolatedAuthToken::createBasicTokenOrNull(user.view(), password.view());

    if (!handle) {
        return AuthToken(JavaObjectHandle<AuthToken>(), true);
    }

    return AuthToken(std::move(handle));
}

AuthToken AuthToken::createBearerToken(StringParam token) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createBearerToken(token.view()));
}

AuthToken AuthToken::createBearerTokenOrNull(StringParam token) {
    auto handle = isolated::auth::IsolatedAuthToken::createBearerTokenOrNull(token.view());

    if (!handle) {
        return AuthToken(JavaObjectHandle<AuthToken>(), true);
    }

    return AuthToken(std::move(handle));
}

AuthToken AuthToken::createCustomToken(StringParam scheme, StringParam value) {
    return AuthToken(isolated::auth::IsolatedAuthToken::createCustomToken(scheme.view(), value.view()));
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

    return JavaObject::equals(handle_.get(), other.handle_.get());
}

AuthToken::AuthToken(JavaObjectHandle<AuthToken> &&handle, bool isNull) : handle_(std::move(handle)), isNull_(isNull) {
}

DXFCPP_END_NAMESPACE