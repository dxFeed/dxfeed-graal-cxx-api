// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../auth/AuthToken.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::auth::IsolatedAuthToken {

/// dxfg_AuthToken_valueOf
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> valueOf(/* const char* */ std::string_view string);

/// dxfg_AuthToken_createBasicToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicToken(/* const char* */ std::string_view userPassword);

/// dxfg_AuthToken_createBasicToken2
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicToken(/* const char* */ std::string_view user, /* const char* */ std::string_view password);

/// dxfg_AuthToken_createBasicTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicTokenOrNull(/* const char* */ std::string_view user, /* const char* */ std::string_view password);

/// dxfg_AuthToken_createBearerToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBearerToken(/* const char* */ std::string_view token);

/// dxfg_AuthToken_createBearerTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBearerTokenOrNull(/* const char* */ std::string_view token);

/// dxfg_AuthToken_createCustomToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createCustomToken(/* const char* */ std::string_view scheme, /* const char* */ std::string_view value);

/// dxfg_AuthToken_getHttpAuthorization
/* const char* */ std::string getHttpAuthorization(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken>& authToken);

/// dxfg_AuthToken_getUser
/* const char* */ std::string getUser(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken>& authToken);

/// dxfg_AuthToken_getPassword
/* const char* */ std::string getPassword(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken>& authToken);

/// dxfg_AuthToken_getScheme
/* const char* */ std::string getScheme(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken>& authToken);

/// dxfg_AuthToken_getValue
/* const char* */ std::string getValue(/* dxfg_auth_token_t* */ const JavaObjectHandle<AuthToken>& authToken);

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()