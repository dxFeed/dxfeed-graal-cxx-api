// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/auth/IsolatedAuthToken.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::auth::IsolatedAuthToken {

/// dxfg_AuthToken_valueOf
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> valueOf(/* const char* */ const StringLike &string) {
    return JavaObjectHandle<AuthToken>(runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_valueOf, string.data()));
}

/// dxfg_AuthToken_createBasicToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createBasicToken(/* const char* */ const StringLike &userPassword) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createBasicToken, userPassword.data()));
}

/// dxfg_AuthToken_createBasicToken2
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createBasicToken(/* const char* */ const StringLike &user,
                 /* const char* */ const StringLike &password) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createBasicToken2, user.data(), password.data()));
}

/// dxfg_AuthToken_createBasicTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createBasicTokenOrNull(/* const char* */ const StringLike &user,
                       /* const char* */ const StringLike &password) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createBasicTokenOrNull, user.data(), password.data()));
}

/// dxfg_AuthToken_createBearerToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createBearerToken(/* const char* */ const StringLike &token) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createBearerToken, token.data()));
}

/// dxfg_AuthToken_createBearerTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createBearerTokenOrNull(/* const char* */ const StringLike &token) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createBearerTokenOrNull, token.data()));
}

/// dxfg_AuthToken_createCustomToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken>
createCustomToken(/* const char* */ const StringLike &scheme,
                  /* const char* */ const StringLike &value) {
    return JavaObjectHandle<AuthToken>(
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_createCustomToken, scheme.data(), value.data()));
}

/// dxfg_AuthToken_getHttpAuthorization
/* const char* */ std::string
getHttpAuthorization(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthToken_getHttpAuthorization`. The `authToken` handle is invalid");
    }

    auto httpAuthorization = runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_getHttpAuthorization,
                                                               static_cast<dxfg_auth_token_t *>(authToken.get()));
    auto result = dxfcpp::toString(httpAuthorization);

    internal::IsolatedString::release(httpAuthorization);

    return result;
}

/// dxfg_AuthToken_getUser
/* const char* */ std::string getUser(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthToken_getUser`. The `authToken` handle is invalid");
    }

    auto user =
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_getUser, static_cast<dxfg_auth_token_t *>(authToken.get()));
    auto result = dxfcpp::toString(user);

    internal::IsolatedString::release(user);

    return result;
}

/// dxfg_AuthToken_getPassword
/* const char* */ std::string getPassword(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthToken_getPassword`. The `authToken` handle is invalid");
    }

    auto password = runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_getPassword,
                                                      static_cast<dxfg_auth_token_t *>(authToken.get()));
    auto result = dxfcpp::toString(password);

    internal::IsolatedString::release(password);

    return result;
}

/// dxfg_AuthToken_getScheme
/* const char* */ std::string getScheme(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthToken_getScheme`. The `authToken` handle is invalid");
    }

    auto scheme =
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_getScheme, static_cast<dxfg_auth_token_t *>(authToken.get()));
    auto result = dxfcpp::toString(scheme);

    internal::IsolatedString::release(scheme);

    return result;
}

/// dxfg_AuthToken_getValue
/* const char* */ std::string getValue(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_AuthToken_getValue`. The `authToken` handle is invalid");
    }

    auto value =
        runGraalFunctionAndThrowIfNullptr(dxfg_AuthToken_getValue, static_cast<dxfg_auth_token_t *>(authToken.get()));
    auto result = dxfcpp::toString(value);

    internal::IsolatedString::release(value);

    return result;
}

} // namespace isolated::auth::IsolatedAuthToken

DXFCPP_END_NAMESPACE