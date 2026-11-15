// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * A connection error of a @ref MessageConnectorInfo "message connector" of an endpoint: a failed connect attempt or a
 * dropped connection (DXEndpoint::getLastConnectionError(), MessageConnectorInfo::lastError).
 *
 * The value is a copy: it does not change when the connector records a newer error. A successful connection clears
 * nothing, the #time tells how old the error is. See @ref connection_troubleshooting "Connection troubleshooting".
 *
 * This is an experimental API, it may change in future versions.
 */
struct DXFCPP_EXPORT ConnectionError {
    /**
     * The category of a connection error. A category that this version of the library does not know reads as
     * @ref Category::OTHER "OTHER".
     */
    enum class Category : std::int32_t {
        /// Another error, or a category that this version of the library does not know.
        OTHER = 0,

        /// The host name of the address is not resolved (Java: `java.net.UnknownHostException`).
        DNS = 1,

        /// The connect failed, was refused or timed out (Java: `java.net.ConnectException`, `NoRouteToHostException`,
        /// `PortUnreachableException`).
        CONNECT_FAILED = 2,

        /// A read of an established connection timed out (Java: `java.net.SocketTimeoutException`).
        READ_TIMEOUT = 3,

        /// The remote side sent no data and no heartbeat within the heartbeat timeout (the address option
        /// `[heartbeatTimeout=...]`), so the connection was closed; the error has no exception.
        HEARTBEAT_TIMEOUT = 4,

        /// The remote side closed the connection (Java: `java.io.EOFException`).
        CLOSED_BY_REMOTE = 5,

        /// The TLS handshake or a TLS record failed, for example a `tls+` address of a port without TLS or a
        /// certificate that is not trusted (Java: `javax.net.ssl.SSLException`).
        TLS = 6,

        /// Another I/O error, for example a connection reset (Java: `java.io.IOException`).
        IO = 7,
    };

    /// The time of the error, in milliseconds since the UNIX epoch (UTC).
    std::int64_t time{};

    /// The address the connector was configured with, credentials masked: for a multi-host address the whole list,
    /// not the host that failed.
    std::string address{};

    /// The category of the error.
    Category category{Category::OTHER};

    /// `true` if an established connection was dropped, `false` if a connect attempt failed.
    bool dropped{};

    /// The class name of the Java exception, for example `java.net.ConnectException`, or an empty string when there is
    /// none.
    std::string exceptionClassName{};

    /// The message of the exception (or of the log record when there is no exception), credentials masked, at most 256
    /// characters; an empty string when there is none.
    std::string message{};

    /// The number of errors of the connector up to this one: it grows with each new error.
    std::int64_t count{};

    /**
     * Returns the name of a category, for example `CONNECT_FAILED`.
     *
     * @param category The category.
     * @return The name of the category.
     */
    static const char *categoryToString(Category category) noexcept;

    /**
     * Returns a string representation of the error in one line: the time, the category, whether the connection was
     * dropped, the address, the exception, the message and the count.
     *
     * @return The string representation of the error.
     */
    std::string toString() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
