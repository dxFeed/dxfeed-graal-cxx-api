// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/Common.hpp"
#include "../internal/JavaObjectHandle.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

class InstrumentProfileReader;

/**
 * The AuthToken class represents an authorization token and encapsulates information about the authorization scheme
 * and its associated value.
 *
 * <p>An AuthToken consists of the following components:
 * <ul>
 *   <li>Scheme - The authorization scheme (e.g., "Basic" or "Bearer").</li>
 *   <li>Value - The encoded value, which is scheme-dependent (e.g., an access token per RFC6750 or Base64-encoded
 * "user:password" per RFC2617).</li> <li>String representation - A string that combines the scheme and value in the
 * format: [scheme + " " + value].</li>
 * </ul>
 */
struct DXFCPP_EXPORT AuthToken {
    friend class InstrumentProfileReader;

    static const std::string BASIC_SCHEME;
    static const std::string BEARER_SCHEME;

    static const AuthToken NULL_TOKEN;

    /**
     * Constructs an AuthToken from the specified string.
     *
     * @param string The string (or c-string or string view) with space-separated scheme and value.
     * @return The constructed AuthToken.
     * @throws JavaException("InvalidFormatException") if the string is malformed, or if the scheme is "Basic" but the
     * format does not comply with RFC2617
     * @see ::toString()
     */
    static AuthToken valueOf(const StringLikeWrapper &string);

    /**
     * Constructs an AuthToken with the specified username and password per RFC2617.
     * Username and password can be empty.
     *
     * @param userPassword the string (or c-string or string view) containing the username and password in the format
     * "username:password"
     * @return The constructed AuthToken.
     * @throws JavaException("InvalidFormatException") if the userPassword is malformed
     */
    static AuthToken createBasicToken(const StringLikeWrapper &userPassword);

    /**
     * Constructs an AuthToken with the specified username and password per RFC2617.
     * Username and password can be empty.
     *
     * @param user The username.
     * @param password The password.
     * @return The constructed AuthToken.
     */
    static AuthToken createBasicToken(const StringLikeWrapper &user, const StringLikeWrapper &password);

    /**
     * Constructs an AuthToken with the specified username and password per RFC2617.
     * If both the username and password are empty returns AuthToken::NULL_TOKEN.
     *
     * @param user the username
     * @param password the password
     * @return the constructed AuthToken or AuthToken::NULL_TOKEN
     */
    static AuthToken createBasicTokenOrNull(const StringLikeWrapper &user, const StringLikeWrapper &password);

    /**
     * Constructs an AuthToken with the specified bearer token per RFC6750.
     *
     * @param token the access token
     * @return the constructed AuthToken
     * @throws JavaException("InvalidFormatException") if the token is empty
     */
    static AuthToken createBearerToken(const StringLikeWrapper &token);

    /**
     * Constructs an AuthToken with the specified bearer token per RFC6750.
     *
     * @param token the access token
     * @return the constructed AuthToken or AuthToken::NULL_TOKEN.
     */
    static AuthToken createBearerTokenOrNull(const StringLikeWrapper &token);

    /**
     * Constructs an AuthToken with a custom scheme and value.
     *
     * @param scheme the custom scheme
     * @param value the custom value
     * @return the constructed AuthToken
     * @throws JavaException("InvalidFormatException") if the scheme or value is empty
     */
    static AuthToken createCustomToken(const StringLikeWrapper &scheme, const StringLikeWrapper &value);

    /**
     * Returns the HTTP authorization header value.
     *
     * @return the HTTP authorization header value
     */
    std::string getHttpAuthorization() const;

    /**
     * Returns the username or dxfcpp::String::NUL (`std::string{"<null>"}`) if it is not known or applicable.
     *
     * @return the username, or dxfcpp::String::NUL (`std::string{"<null>"}`) if not known or applicable
     */
    std::string getUser() const;

    /**
     * Returns the password or dxfcpp::String::NUL (`std::string{"<null>"}`) if it is not known or applicable.
     *
     * @return the password, or dxfcpp::String::NUL (`std::string{"<null>"}`) if not known or applicable
     */
    std::string getPassword() const;

    /**
     * Returns the authentication scheme.
     *
     * @return the authentication scheme
     */
    std::string getScheme() const;

    /**
     * Returns the access token for RFC6750 or the Base64-encoded "username:password" for RFC2617.
     *
     * @return the access token or Base64-encoded "username:password"
     */
    std::string getValue() const;

    std::string toString() const;

    friend std::ostream &operator<<(std::ostream &os, const AuthToken &t) {
        return os << t.toString();
    }

    std::size_t hashCode() const;

    bool operator==(const AuthToken &other) const noexcept;

    virtual ~AuthToken() noexcept = default;

    AuthToken(const AuthToken &) = delete;
    AuthToken(AuthToken &&) noexcept = delete;
    AuthToken &operator=(const AuthToken &) = delete;
    AuthToken &operator=(const AuthToken &&) noexcept = delete;

private:

    JavaObjectHandle<AuthToken> handle_;
    std::atomic<bool> isNull_;

    explicit AuthToken(JavaObjectHandle<AuthToken> &&handle, bool isNull = false);
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::AuthToken> {
    std::size_t operator()(const dxfcpp::AuthToken &t) const noexcept {
        return t.hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()