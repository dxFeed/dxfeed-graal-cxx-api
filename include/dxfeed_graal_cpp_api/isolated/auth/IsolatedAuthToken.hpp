// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../auth/AuthToken.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::auth::IsolatedAuthToken {

/// dxfg_AuthToken_valueOf
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> valueOf(/* const char* */ const StringLikeWrapper & string);

/// dxfg_AuthToken_createBasicToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicToken(/* const char* */ const StringLikeWrapper & userPassword);

/// dxfg_AuthToken_createBasicToken2
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicToken(/* const char* */ const StringLikeWrapper & user, /* const char* */ const StringLikeWrapper & password);

/// dxfg_AuthToken_createBasicTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBasicTokenOrNull(/* const char* */ const StringLikeWrapper & user, /* const char* */ const StringLikeWrapper & password);

/// dxfg_AuthToken_createBearerToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBearerToken(/* const char* */ const StringLikeWrapper & token);

/// dxfg_AuthToken_createBearerTokenOrNull
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createBearerTokenOrNull(/* const char* */ const StringLikeWrapper & token);

/// dxfg_AuthToken_createCustomToken
/* dxfg_auth_token_t* */ JavaObjectHandle<AuthToken> createCustomToken(/* const char* */ const StringLikeWrapper & scheme, /* const char* */ const StringLikeWrapper & value);

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