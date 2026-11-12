// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "../internal/InternalExport.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

struct DXEndpoint;

/**
 * The raw description of the request that created a promise of DXFeed. The text is built lazily, when the wait of the
 * promise times out: see describePromiseTimeout().
 */
struct PromiseContext {
    /// The name of the DXFeed method, for example `getLastEventPromise`.
    std::string method{};

    /// The name of the event type, for example `Quote`.
    std::string eventType{};

    /// The symbol of the request.
    std::string symbol{};

    /// The source of the request (indexed events only).
    std::optional<std::string> source{};

    /// The beginning of the time range (time series only).
    std::optional<std::int64_t> fromTime{};

    /// The end of the time range (time series only).
    std::optional<std::int64_t> toTime{};

    /// The endpoint that created the feed of the promise. It is weak: the description does not keep the endpoint alive.
    std::weak_ptr<const DXEndpoint> endpoint{};
};

/**
 * Builds the text that is added to the message of the Java exception when the wait of the promise times out:
 * `timed out after <elapsed> ms (timeout <timeout> ms) waiting for <method><<Type>>(<symbol>[, source=<source>]
 * [, fromTime=<time>, toTime=<time>]); endpoint state: <STATE|unknown>`.
 *
 * Does not throw because of the formatting of the times or the state of the endpoint: a time falls back to the number
 * of milliseconds, the state to `unknown`.
 *
 * @param context The description of the request.
 * @param elapsedInMilliseconds The measured time of the wait.
 * @param timeoutInMilliseconds The timeout of the wait.
 * @return The text.
 */
DXFCPP_INTERNAL_EXPORT std::string describePromiseTimeout(const PromiseContext &context,
                                                          std::int64_t elapsedInMilliseconds,
                                                          std::int32_t timeoutInMilliseconds);

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
