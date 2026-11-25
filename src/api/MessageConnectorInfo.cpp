// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/MessageConnectorInfo.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <string>

DXFCPP_BEGIN_NAMESPACE

const char *MessageConnectorInfo::kindToString(Kind kind) noexcept {
    switch (kind) {
    case Kind::CLIENT_SOCKET:
        return "CLIENT_SOCKET";
    case Kind::SERVER_SOCKET:
        return "SERVER_SOCKET";
    case Kind::OTHER:
        return "OTHER";
    }

    return "OTHER";
}

const char *MessageConnectorInfo::stateToString(State state) noexcept {
    switch (state) {
    case State::CONNECTING:
        return "CONNECTING";
    case State::CONNECTED:
        return "CONNECTED";
    case State::DISCONNECTED:
        return "DISCONNECTED";
    }

    return "DISCONNECTED";
}

std::string MessageConnectorInfo::toString() const {
    return fmt::format("MessageConnectorInfo{{name = {}, kind = {}, state = {}, active = {}, address = {}, "
                       "currentAddresses = [{}], connectionCount = {}, closedConnectionCount = {}, readBytes = {}, "
                       "writtenBytes = {}, lastError = {}}}",
                       name, kindToString(kind), stateToString(state), active, address,
                       fmt::join(currentAddresses, ", "), connectionCount, closedConnectionCount, statistics.readBytes,
                       statistics.writtenBytes, lastError ? lastError->toString() : std::string{"none"});
}

DXFCPP_END_NAMESPACE
