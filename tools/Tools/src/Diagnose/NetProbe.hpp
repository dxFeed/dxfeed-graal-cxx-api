// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace dxfcpp::tools::diagnose {

/// The result of a TCP connect to one IP address.
struct IpProbe {
    std::string ip{};
    bool connected{};
    /// The time of the connect or of the failure.
    std::int64_t millis{};
    /// The error when the connect failed.
    std::string error{};
};

/// The result of the probe of a host and port: the IP addresses of the host and a TCP connect to each.
struct HostProbe {
    std::string host{};
    std::uint16_t port{};
    bool resolved{};
    /// The error when the host name was not resolved.
    std::string error{};
    std::vector<IpProbe> ips{};
};

/// The maximum number of IP addresses of a host that the probe connects to.
inline constexpr std::size_t MAX_PROBED_IPS = 16;

/**
 * Resolves the host to all its IP addresses (IPv4 and IPv6) and tries a TCP connect to each of them (at most
 * MAX_PROBED_IPS), one at a time, each within the timeout. The connections are closed at once; nothing is sent.
 */
HostProbe probeHost(const std::string &host, std::uint16_t port, std::chrono::milliseconds timeout);

} // namespace dxfcpp::tools::diagnose
