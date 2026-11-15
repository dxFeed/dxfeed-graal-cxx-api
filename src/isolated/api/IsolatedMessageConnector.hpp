// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/api/ConnectionError.hpp"
#include "../../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../../../include/dxfeed_graal_cpp_api/api/MessageConnectorInfo.hpp"
#include "../../../include/dxfeed_graal_cpp_api/detail/JavaObjectHandle.hpp"

#include <dxfg_api.h>

#include <cstdint>
#include <optional>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

// The message connectors of an endpoint and their connection errors (dxfg_v4_message_connector.h). The functions of the
// connection errors and of the connector listener are experimental: each wrapper that calls one checks the advanced
// revision of the SDK library first (Isolate::checkAdvancedRevision()).
namespace isolated::api::IsolatedMessageConnector {

/// Maps a `DXFG_V4_CONNECTION_ERROR_*` value; an unknown value is ConnectionError::Category::OTHER.
constexpr ConnectionError::Category graalCategoryToCategory(std::int32_t category) noexcept {
    switch (category) {
    case DXFG_V4_CONNECTION_ERROR_DNS:
        return ConnectionError::Category::DNS;
    case DXFG_V4_CONNECTION_ERROR_CONNECT_FAILED:
        return ConnectionError::Category::CONNECT_FAILED;
    case DXFG_V4_CONNECTION_ERROR_READ_TIMEOUT:
        return ConnectionError::Category::READ_TIMEOUT;
    case DXFG_V4_CONNECTION_ERROR_HEARTBEAT_TIMEOUT:
        return ConnectionError::Category::HEARTBEAT_TIMEOUT;
    case DXFG_V4_CONNECTION_ERROR_CLOSED_BY_REMOTE:
        return ConnectionError::Category::CLOSED_BY_REMOTE;
    case DXFG_V4_CONNECTION_ERROR_TLS:
        return ConnectionError::Category::TLS;
    case DXFG_V4_CONNECTION_ERROR_IO:
        return ConnectionError::Category::IO;
    default:
        return ConnectionError::Category::OTHER;
    }
}

/// Maps a `DXFG_V4_MESSAGE_CONNECTOR_KIND_*` value; an unknown value is MessageConnectorInfo::Kind::OTHER.
constexpr MessageConnectorInfo::Kind graalKindToKind(std::int32_t kind) noexcept {
    switch (kind) {
    case DXFG_V4_MESSAGE_CONNECTOR_KIND_CLIENT_SOCKET:
        return MessageConnectorInfo::Kind::CLIENT_SOCKET;
    case DXFG_V4_MESSAGE_CONNECTOR_KIND_SERVER_SOCKET:
        return MessageConnectorInfo::Kind::SERVER_SOCKET;
    default:
        return MessageConnectorInfo::Kind::OTHER;
    }
}

/// Maps a `DXFG_V4_MESSAGE_CONNECTOR_STATE_*` value; an unknown value is MessageConnectorInfo::State::DISCONNECTED.
constexpr MessageConnectorInfo::State graalStateToState(std::int32_t state) noexcept {
    switch (state) {
    case DXFG_V4_MESSAGE_CONNECTOR_STATE_CONNECTING:
        return MessageConnectorInfo::State::CONNECTING;
    case DXFG_V4_MESSAGE_CONNECTOR_STATE_CONNECTED:
        return MessageConnectorInfo::State::CONNECTED;
    default:
        return MessageConnectorInfo::State::DISCONNECTED;
    }
}

/**
 * Reads a snapshot of a connector: its values one by one and its last error (the experimental
 * `dxfg_v4_MessageConnector_getLastError`).
 *
 * @param connector The connector's handle (`dxfg_v4_message_connector_t *`); valid during the call.
 * @return The snapshot.
 * @throws InvalidArgumentException if the connector handle is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
MessageConnectorInfo getInfo(/* dxfg_v4_message_connector_t * */ void *connector);

/**
 * Calls `dxfg_v4_DXEndpoint_getConnectors` and reads a snapshot of each connector (getInfo()).
 *
 * @param endpoint The endpoint's handle.
 * @return The snapshots of the connectors; empty when the endpoint is not connected or is closed.
 * @throws InvalidArgumentException if the endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
std::vector<MessageConnectorInfo> getConnectors(const JavaObjectHandle<DXEndpoint> &endpoint);

/**
 * Calls the experimental `dxfg_v4_DXEndpoint_getLastConnectionError`.
 *
 * @param endpoint The endpoint's handle.
 * @return The last connection error of the endpoint, or `std::nullopt` when it has had none.
 * @throws InvalidArgumentException if the endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
std::optional<ConnectionError> getLastConnectionError(const JavaObjectHandle<DXEndpoint> &endpoint);

namespace Listener {

/**
 * Calls the experimental `dxfg_v4_MessageConnectorListener_new`.
 *
 * @param userFunc The callback (`dxfg_v4_message_connector_listener_func`).
 * @param userData The user data passed to each call of the callback.
 * @return The listener's handle.
 * @throws InvalidArgumentException if userFunc is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
JavaObjectHandle<DXEndpointMessageConnectorListener>
create(/* dxfg_v4_message_connector_listener_func */ void *userFunc, void *userData);

/**
 * Calls the experimental `dxfg_v4_DXEndpoint_addMessageConnectorListener`.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if the endpoint handle or the listener handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
void add(const JavaObjectHandle<DXEndpoint> &endpoint,
         const JavaObjectHandle<DXEndpointMessageConnectorListener> &listener);

/**
 * Calls the experimental `dxfg_v4_DXEndpoint_removeMessageConnectorListener`. It does not wait for a call of the
 * listener in progress; releasing the last handle of the listener does.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if the endpoint handle or the listener handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM, or the advanced revision of the SDK library differs.
 */
void remove(const JavaObjectHandle<DXEndpoint> &endpoint,
            const JavaObjectHandle<DXEndpointMessageConnectorListener> &listener);

} // namespace Listener

} // namespace isolated::api::IsolatedMessageConnector

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
