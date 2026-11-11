// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedAuthToken.hpp"

#include "../../../include/dxfeed_graal_cpp_api/auth/AuthToken.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::auth::IsolatedAuthToken {

/// dxfg_v4_AuthToken_valueOf
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken> valueOf(/* const char* */ std::string_view string) {
    return JavaObjectHandle<AuthToken>(
        runGraalV4FunctionWithNonNullResult(
            dxfg_v4_AuthToken_valueOf,
            SdkString{string, StringLabel::secretOf("AuthToken::valueOf(string)")}.c_str()));
}

/// dxfg_v4_AuthToken_createBasicToken
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken>
createBasicToken(/* const char* */ std::string_view userPassword) {
    return JavaObjectHandle<AuthToken>(
        runGraalV4FunctionWithNonNullResult(
            dxfg_v4_AuthToken_createBasicToken,
            SdkString{userPassword, StringLabel::secretOf("AuthToken::createBasicToken(userPassword)")}.c_str()));
}

/// dxfg_v4_AuthToken_createBasicTokenWithUserAndPassword
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicToken(/* const char* */ std::string_view user,
                                                                         /* const char* */ std::string_view password) {
    return JavaObjectHandle<AuthToken>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_AuthToken_createBasicTokenWithUserAndPassword,
        SdkString{user, StringLabel::secretOf("AuthToken::createBasicToken(user)")}.c_str(),
        SdkString{password, StringLabel::secretOf("AuthToken::createBasicToken(password)")}.c_str()));
}

/// dxfg_v4_AuthToken_createBasicTokenOrNull
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken>
createBasicTokenOrNull(/* const char* */ std::string_view user,
                       /* const char* */ std::string_view password) {
    return JavaObjectHandle<AuthToken>(runGraalV4FunctionWithResult(
        dxfg_v4_AuthToken_createBasicTokenOrNull,
        SdkString{user, StringLabel::secretOf("AuthToken::createBasicTokenOrNull(user)")}.c_str(),
        SdkString{password, StringLabel::secretOf("AuthToken::createBasicTokenOrNull(password)")}.c_str()));
}

/// dxfg_v4_AuthToken_createBearerToken
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken> createBearerToken(/* const char* */ std::string_view token) {
    return JavaObjectHandle<AuthToken>(
        runGraalV4FunctionWithNonNullResult(
            dxfg_v4_AuthToken_createBearerToken,
            SdkString{token, StringLabel::secretOf("AuthToken::createBearerToken(token)")}.c_str()));
}

/// dxfg_v4_AuthToken_createBearerTokenOrNull
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken>
createBearerTokenOrNull(/* const char* */ std::string_view token) {
    return JavaObjectHandle<AuthToken>(
        runGraalV4FunctionWithResult(
            dxfg_v4_AuthToken_createBearerTokenOrNull,
            SdkString{token, StringLabel::secretOf("AuthToken::createBearerTokenOrNull(token)")}.c_str()));
}

/// dxfg_v4_AuthToken_createCustomToken
/* dxfg_v4_auth_token_t* */ JavaObjectHandle<AuthToken> createCustomToken(/* const char* */ std::string_view scheme,
                                                                          /* const char* */ std::string_view value) {
    return JavaObjectHandle<AuthToken>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_AuthToken_createCustomToken,
        SdkString{scheme, "AuthToken::createCustomToken(scheme)"}.c_str(),
        SdkString{value, StringLabel::secretOf("AuthToken::createCustomToken(value)")}.c_str()));
}

/// dxfg_v4_AuthToken_getHttpAuthorization
/* const char* */ std::string
getHttpAuthorization(/* dxfg_v4_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthToken_getHttpAuthorization`. The `authToken` handle is invalid");
    }

    const auto httpAuthorization = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_AuthToken_getHttpAuthorization, static_cast<dxfg_v4_auth_token_t *>(authToken.get())));
    return dxfcpp::toString(httpAuthorization.get());
}

/// dxfg_v4_AuthToken_getUser
/* const char* */ std::string getUser(/* dxfg_v4_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthToken_getUser`. The `authToken` handle is invalid");
    }

    const auto user = internal::IsolatedString::toUnique(
        runGraalV4FunctionWithResult(dxfg_v4_AuthToken_getUser, static_cast<dxfg_v4_auth_token_t *>(authToken.get())));
    return dxfcpp::toString(user.get());
}

/// dxfg_v4_AuthToken_getPassword
/* const char* */ std::string getPassword(/* dxfg_v4_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthToken_getPassword`. The `authToken` handle is invalid");
    }

    const auto password = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_AuthToken_getPassword, static_cast<dxfg_v4_auth_token_t *>(authToken.get())));
    return dxfcpp::toString(password.get());
}

/// dxfg_v4_AuthToken_getScheme
/* const char* */ std::string getScheme(/* dxfg_v4_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthToken_getScheme`. The `authToken` handle is invalid");
    }

    const auto scheme = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_AuthToken_getScheme, static_cast<dxfg_v4_auth_token_t *>(authToken.get())));
    return dxfcpp::toString(scheme.get());
}

/// dxfg_v4_AuthToken_getValue
/* const char* */ std::string getValue(/* dxfg_v4_auth_token_t* */ const JavaObjectHandle<AuthToken> &authToken) {
    if (!authToken) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_AuthToken_getValue`. The `authToken` handle is invalid");
    }

    const auto value = internal::IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_AuthToken_getValue, static_cast<dxfg_v4_auth_token_t *>(authToken.get())));
    return dxfcpp::toString(value.get());
}

} // namespace isolated::auth::IsolatedAuthToken

DXFCPP_END_NAMESPACE
