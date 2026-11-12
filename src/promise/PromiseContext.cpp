// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "PromiseContext.hpp"

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"
#include "../internal/utils/StringUtils.hpp"

#include <limits>
#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace {

// Formatting a time is a call to the Java side: on failure the time is the number of milliseconds.
std::string formatTime(std::int64_t timestamp) {
    if (timestamp == std::numeric_limits<std::int64_t>::max()) {
        return "max";
    }

    try {
        return formatTimeStampWithMillis(timestamp);
    } catch (...) {
        return std::to_string(timestamp);
    }
}

// Reading the state is a call to the Java side: it fails, for example, after the shutdown.
std::string describeEndpointState(const std::weak_ptr<const DXEndpoint> &weakEndpoint) {
    try {
        if (const auto endpoint = weakEndpoint.lock()) {
            return DXEndpoint::stateToString(endpoint->getState());
        }
    } catch (...) {
    }

    return "unknown";
}

std::string describeRequest(const PromiseContext &context) {
    std::string result = context.method + "<" + context.eventType + ">(" + context.symbol;

    if (context.source) {
        result += ", source=" + *context.source;
    }

    if (context.fromTime && context.toTime) {
        result += ", fromTime=" + formatTime(*context.fromTime) + ", toTime=" + formatTime(*context.toTime);
    }

    return result + ")";
}

} // namespace

std::string describePromiseTimeout(const PromiseContext &context, std::int64_t elapsedInMilliseconds,
                                   std::int32_t timeoutInMilliseconds) {
    return "timed out after " + std::to_string(elapsedInMilliseconds) + " ms (timeout " +
           std::to_string(timeoutInMilliseconds) + " ms) waiting for " + describeRequest(context) +
           "; endpoint state: " + describeEndpointState(context.endpoint);
}

DXFCPP_END_NAMESPACE
