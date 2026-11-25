// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "./ConnectionError.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @page connection_troubleshooting Connection troubleshooting
 *
 * When an endpoint does not connect, or its connection drops, the endpoint tells why (an experimental API, it may
 * change in future versions):
 *
 * - DXEndpoint::getLastConnectionError() returns the newest connection error of the endpoint: the time, the
 *   @ref ConnectionError::Category "category", the address, whether an established connection dropped, the Java
 *   exception, its message and the number of errors so far.
 * - DXEndpoint::getConnectors() returns a snapshot of each message connector: its state, the addresses it is connected
 *   to now, the numbers of the open and the closed connections, the statistics and its last error.
 * - DXEndpoint::addMessageConnectorListener() adds a listener called after a connector changes its state. The error of
 *   the change is recorded before the call, so the listener reads it from MessageConnectorInfo::lastError.
 *
 * ```cpp
 * endpoint->addMessageConnectorListener([](const MessageConnectorInfo &connector) {
 *     if (connector.lastError) {
 *         std::cerr << connector.lastError->toString() << std::endl;
 *     }
 * });
 * endpoint->connect("demo.dxfeed.com:7300");
 * ```
 *
 * A successful connection clears no error: compare the time or the count of an error with those seen before. A state
 * change listener of the endpoint is not the place to read the error: the state of the endpoint is computed on another
 * thread, so the listener may be called before the error is recorded, and failed connect attempts do not change the
 * state at all: the endpoint stays @ref DXEndpoint::State::CONNECTING "CONNECTING" between them.
 *
 * The categories and their usual causes:
 *
 * | Category | Usual causes |
 * |----------|--------------|
 * | @ref ConnectionError::Category::DNS "DNS" | A mistake in the host name; no DNS server is reachable. |
 * | @ref ConnectionError::Category::CONNECT_FAILED "CONNECT_FAILED" | A wrong port; nothing listens on it; a firewall; the host is down or unreachable. |
 * | @ref ConnectionError::Category::TLS "TLS" | A `tls+` address of a port without TLS; a certificate that is not trusted. |
 * | @ref ConnectionError::Category::CLOSED_BY_REMOTE "CLOSED_BY_REMOTE" | The remote side closed the connection, for example on its restart. |
 * | @ref ConnectionError::Category::HEARTBEAT_TIMEOUT "HEARTBEAT_TIMEOUT" | The network or the remote side stalled: no data and no heartbeat came in time. |
 * | @ref ConnectionError::Category::READ_TIMEOUT "READ_TIMEOUT", @ref ConnectionError::Category::IO "IO" | Network problems, for example a connection reset. |
 * | @ref ConnectionError::Category::OTHER "OTHER" | Other errors, for example of the protocol or of the authentication: see the message. |
 *
 * DXEndpoint::connect() rejects a `ws://` or `wss://` URL and an `http://` or `https://` URL (except a configuration
 * URL `.../name.config#name`) at once: such a URL is not an address of an endpoint. The `Connect` tool prints each new connection error to the standard error stream, one line per error.
 * The errors of the library itself (the exceptions of the listeners and the like) go to ErrorManager: see
 * @ref error_handling "Error handling".
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * A snapshot of a message connector of an endpoint: a connection to an address (DXEndpoint::getConnectors(),
 * DXEndpoint::addMessageConnectorListener()).
 *
 * The values are read one by one when the snapshot is taken, so they are not a consistent set: a connection that is
 * established or closed meanwhile may be counted in one value and not in another. The snapshot does not change
 * afterwards; take a new one to see the changes. See @ref connection_troubleshooting "Connection troubleshooting".
 *
 * This is an experimental API, it may change in future versions.
 */
struct DXFCPP_EXPORT MessageConnectorInfo {
    /// The kind of a connector. A kind that this version of the library does not know reads as @ref Kind::OTHER "OTHER".
    enum class Kind : std::int32_t {
        /// A client socket connector (an address `host:port`).
        CLIENT_SOCKET = 0,

        /// A server socket connector (an address `:port`).
        SERVER_SOCKET = 1,

        /// Any other connector (a file, a non-blocking or a custom connector), or a kind that this version of the
        /// library does not know.
        OTHER = 2,
    };

    /// The state of a connector. A state that this version of the library does not know reads as
    /// @ref State::DISCONNECTED "DISCONNECTED".
    enum class State : std::int32_t {
        /// Started, not connected yet, or reconnecting after a failure or a drop.
        CONNECTING = 0,

        /// At least one of its connections is established (a server socket connector: it accepts connections).
        CONNECTED = 1,

        /// Stopped, for example replaced by a connect to another address, or its endpoint disconnected or closed.
        DISCONNECTED = 2,
    };

    /// The statistics of a connector since it was created.
    struct Statistics {
        /// The number of bytes read.
        std::int64_t readBytes{};

        /// The number of bytes written.
        std::int64_t writtenBytes{};

        /// The number of data records read.
        std::int64_t readDataRecords{};

        /// The number of data records written.
        std::int64_t writtenDataRecords{};

        /// The number of the open connections.
        std::int64_t activeConnectionCount{};

        /// The number of the closed connections.
        std::int64_t closedConnectionCount{};
    };

    /**
     * The I/O statistics of a connector since it was created: the cumulative counters of QD's statistics, with the
     * lags and the round-trip times of the records.
     *
     * The averages of an interval come from the differences of two snapshots:
     * - `readDataLagSumMicros / readDataRecords` is the lag of the data received: the lag that the peer reported in its
     *   last heartbeat (how long the data waited in the peer and upstream of it) plus the round-trip time of the
     *   connection. It means something on a feed; on a publisher it stays 0.
     * - `writtenDataLagSumMicros / writtenDataRecords` is the time a record spent in this process; on a publisher it is
     *   close to 0 (the composing time only).
     * - `readRttSumMicros / (readDataRecords + readSubRecords)` and
     *   `writtenRttSumMicros / (writtenDataRecords + writtenSubRecords)` are the round-trip time of the connections.
     *
     * An interval without records has no average. The round-trip time is 0 until the first heartbeats have gone both
     * ways (about a second after the connect), and a lag or round-trip time of more than about 537 s comes out wrong
     * (QD's time marks wrap every 2^30 microseconds). On iOS the ten I/O counters stay 0. A connector stopped by a
     * connect to another address, by DXEndpoint::disconnect() or by the close of its endpoint reads 0 for the ten I/O
     * counters; the connection counts stay. The bytes are those of the QTP messages, heartbeats included, and may
     * differ from the socket bytes of @ref Statistics, for example with TLS.
     */
    struct IoStatistics {
        /// The number of data records read: it grows only when data arrives.
        std::int64_t readDataRecords{};

        /// The number of data records written.
        std::int64_t writtenDataRecords{};

        /// The number of subscription records read: those a publisher receives.
        std::int64_t readSubRecords{};

        /// The number of subscription records written: those a feed sends.
        std::int64_t writtenSubRecords{};

        /// The number of bytes of the QTP messages read, heartbeats included.
        std::int64_t readBytes{};

        /// The number of bytes of the QTP messages written, heartbeats included.
        std::int64_t writtenBytes{};

        /// The sum over the data records read of their lag, in microseconds.
        std::int64_t readDataLagSumMicros{};

        /// The sum over the data records written of their lag, in microseconds.
        std::int64_t writtenDataLagSumMicros{};

        /// The sum over the records read of the round-trip time of the connection, in microseconds.
        std::int64_t readRttSumMicros{};

        /// The sum over the records written of the round-trip time of the connection, in microseconds.
        std::int64_t writtenRttSumMicros{};

        /// The number of the open connections.
        std::int64_t activeConnectionCount{};

        /// The number of the closed connections.
        std::int64_t closedConnectionCount{};
    };

    /// The name of the connector, as in the log of QD, for example `ClientSocket-Distributor`. The connectors of
    /// different endpoints may have the same name.
    std::string name{};

    /// The kind of the connector.
    Kind kind{Kind::OTHER};

    /// The address the connector was configured with, for example `127.0.0.1:7300`, credentials masked. A server
    /// socket connector of the address `:7400` has `*:7400`.
    std::string address{};

    /// The `host:port` addresses a client socket connector is connected to now, one per connection (a striped
    /// connector has several). Empty when the connector is not connected, and always empty for the other kinds.
    std::vector<std::string> currentAddresses{};

    /// The state of the connector.
    State state{State::DISCONNECTED};

    /// `true` if the connector is started (it connects or reconnects), `false` if it is stopped.
    bool active{};

    /// The number of the open connections: those of a client socket connector (more than one for a striped connector)
    /// or those that a server socket connector accepted.
    std::int32_t connectionCount{};

    /// The number of the connections closed since the connector was created: it grows on each reconnect.
    std::int64_t closedConnectionCount{};

    /// The statistics of the connector.
    Statistics statistics{};

    /// The I/O statistics of the connector, or `std::nullopt` when the Graal Native SDK library does not support them.
    std::optional<IoStatistics> ioStatistics{};

    /// The last connection error of the connector, or `std::nullopt` when it has had none.
    std::optional<ConnectionError> lastError{};

    /**
     * Returns the name of a kind, for example `CLIENT_SOCKET`.
     *
     * @param kind The kind.
     * @return The name of the kind.
     */
    static const char *kindToString(Kind kind) noexcept;

    /**
     * Returns the name of a state, for example `CONNECTED`.
     *
     * @param state The state.
     * @return The name of the state.
     */
    static const char *stateToString(State state) noexcept;

    /**
     * Returns a string representation of the connector in one line: the name, the kind, the state, the address, the
     * current addresses, the counts of the connections and the last error.
     *
     * @return The string representation of the connector.
     */
    std::string toString() const;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
