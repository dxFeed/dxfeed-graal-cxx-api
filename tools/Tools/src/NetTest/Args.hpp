// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The command line of NetTest: the options, their parsing and validation, the usage text and the system properties.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace dxfcpp::tools::nettest {

/// The version of the NetTest specification that the tool follows; --help prints it as `spec_version nettest/1`.
inline constexpr const char *SPEC_VERSION = "nettest/1";

/// The implementation of the tool: the `implementation` of the summary and the label `tool` of /metrics.
inline constexpr const char *IMPLEMENTATION = "cpp-api";

/// The mode of the run: the first argument, `p` or `c`.
enum class Mode {
    PRODUCER,
    CONSUMER,
};

/// The event type that the instances publish or receive.
enum class EventKind {
    TRADE,
    TIME_AND_SALE,
};

/// The contract of -c: what the consumer receives, and through it the default roles of the endpoints.
enum class Collector {
    STREAM,
    TICKER,
};

/// The role of an endpoint (the roles that the modes of NetTest use).
enum class Role {
    FEED,
    STREAM_FEED,
    PUBLISHER,
    STREAM_PUBLISHER,
};

/// The parsed and validated command line.
struct Settings {
    Mode mode = Mode::PRODUCER;
    /// --help was given: print the usage to stdout and exit with 0; the other fields are not validated then.
    bool help = false;
    /// The address of the endpoints: `host:port` to connect, `:port` to listen.
    std::string address{};
    /// The number of the symbols of the set (the `<total>` of -S).
    std::int64_t totalSymbols = 0;
    /// The IPF file of the symbols (the `@<file>` of -S), empty when the symbols are generated.
    std::string symbolsFile{};
    /// The number of the instances (-C).
    std::int32_t instances = 1;
    /// The total rate of all instances in records per second (-r */<rate>), unset when the generators run flat out.
    std::optional<std::int64_t> recordsPerSecond{};
    /// The period of the statistics line in milliseconds (-s).
    std::int64_t periodMillis = 10'000;
    /// The name in the statistics line and the base of the endpoint names (-n).
    std::string name = "NetTest";
    /// The number of the events of a published batch (--batch).
    std::int32_t batch = 100;
    EventKind event = EventKind::TRADE;
    /// The contract (-c), unset without -c.
    std::optional<Collector> collector{};
    /// The role of the endpoints given by --role, unset when -c selects it (see endpointRole()).
    std::optional<Role> role{};
    /// Republish the pre-built events in place instead of copying them (--reuse).
    bool reuse = false;
    /// The length of the run in milliseconds (--duration), unset when it ends on a signal only.
    std::optional<std::int64_t> durationMillis{};
    /// The warm-up in milliseconds (--warmup), unset without one: on `c` it starts at the first received record, on `p`
    /// at the first batch.
    std::optional<std::int64_t> warmupMillis{};
    /// The summary file (--summary), empty without one.
    std::string summaryFile{};
    /// The host and the port of /metrics (--metrics-port [<host>:]<port>); the port is unset without the option, and 0
    /// takes a free port.
    std::string metricsHost = "127.0.0.1";
    std::optional<std::int32_t> metricsPort{};
    /// The arguments (from the mode on) as they were given, for the summary file.
    std::vector<std::string> arguments{};
    /// The system properties (-D<key>=<value>) in the command line order.
    std::vector<std::pair<std::string, std::string>> properties{};
};

/// The result of the parsing: the settings, or the message of the first error.
struct ParsedSettings {
    bool ok = false;
    Settings settings{};
    std::string error{};
};

/// Parses the command line; args are the arguments after the name of the tool, the first one is the mode. Never throws
/// on bad input.
ParsedSettings parseArguments(const std::vector<std::string> &args);

/// Parses a duration with the suffix `ms`, `s` or `m` (for example `500ms`, `10s`, `2m`) into milliseconds. Returns
/// nullopt for a missing suffix, a non-positive or an overflowing value.
std::optional<std::int64_t> parseDuration(const std::string &text);

/// Parses a rate with the optional suffix `k`, `M` or `G` (for example `500k`, `1M`, `2.5M`) into records per second.
/// Returns nullopt for a value below 1 record per second or an overflowing value.
std::optional<std::int64_t> parseRate(const std::string &text);

/// Whether a file can be created in the directory of the path (the current directory for a bare file name).
bool isWritableDirectoryOf(const std::string &file);

/// Returns true when the address makes an endpoint listen (it starts with `:`).
bool isServerAddress(const std::string &address);

/// The role of the endpoints: --role when given, else the role that -c selects for the mode (PUBLISHER or
/// STREAM_PUBLISHER on `p`, FEED or STREAM_FEED on `c`).
Role endpointRole(const Settings &settings);

/// The name of a role as the specification writes it (`PUBLISHER`, `STREAM_FEED`, ...).
std::string roleName(Role role);

/// The name of a contract (`stream`, `ticker`).
std::string collectorName(Collector collector);

/// The name of an event type (`Trade`, `TimeAndSale`).
std::string eventName(EventKind event);

/// Whether the consumer receives conflated events: the role FEED with Trade, which it receives through the ticker
/// contract. Always false on `p`.
bool isConflated(const Settings &settings);

/// The usage text with the line `spec_version nettest/1`.
std::string usageText();

/// The property of the nanosecond part of the event time, which the tool sets on the builder of every endpoint.
inline constexpr const char *NANO_TIME_PROPERTY = "dxscheme.nanoTime";

/// The value of NANO_TIME_PROPERTY for the builders: the last -Ddxscheme.nanoTime of the command line, "true" without
/// one.
std::string endpointNanoTime(const Settings &settings);

/// Whether the endpoints carry the nanosecond part: endpointNanoTime() is `true` in any case, as Java's
/// Boolean.parseBoolean reads it.
bool isNanoTimeOn(const Settings &settings);

} // namespace dxfcpp::tools::nettest
