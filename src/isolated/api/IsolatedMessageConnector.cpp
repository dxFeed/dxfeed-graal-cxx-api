// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedMessageConnector.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_v4_api.h>

#include <string>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedMessageConnector {

namespace {

dxfg_v4_endpoint_t *toGraal(const JavaObjectHandle<DXEndpoint> &endpoint, const char *function) {
    if (!endpoint) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `endpoint` handle is invalid");
    }

    return static_cast<dxfg_v4_endpoint_t *>(endpoint.get());
}

dxfg_v4_message_connector_listener_t *toGraal(const JavaObjectHandle<DXEndpointMessageConnectorListener> &listener,
                                              const char *function) {
    if (!listener) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `listener` handle is invalid");
    }

    return static_cast<dxfg_v4_message_connector_listener_t *>(listener.get());
}

// A NULL string of the SDK is an empty string.
std::string copyString(const char *string) {
    return string == nullptr ? std::string{} : std::string{string};
}

// Takes a string that the SDK allocated: copies it and releases it.
std::string takeString(const char *string) {
    const auto unique = internal::IsolatedString::toUnique(string);

    return copyString(unique.get());
}

// Takes an error that the SDK allocated: copies it and releases it. NULL is no error.
std::optional<ConnectionError> takeError(dxfg_v4_connection_error_t *error) {
    if (error == nullptr) {
        return std::nullopt;
    }

    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_ConnectionError_release, error);
    });

    ConnectionError result{};

    result.time = error->time;
    result.address = copyString(error->address);
    result.category = graalCategoryToCategory(error->category);
    result.dropped = error->dropped != 0;
    result.exceptionClassName = copyString(error->exception_class_name);
    result.message = copyString(error->message);
    result.count = error->count;

    return result;
}

std::vector<std::string> takeStringList(dxfg_v4_string_list *list) {
    std::vector<std::string> result{};

    if (list == nullptr) {
        return result;
    }

    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_string_list_release, list);
    });

    if (list->size > 0) {
        result.reserve(static_cast<std::size_t>(list->size));
    }

    for (std::int32_t i = 0; i < list->size; i++) {
        result.emplace_back(copyString(list->elements[i]));
    }

    return result;
}

} // namespace

// getInfo() maps the 12 values of the I/O statistics one to one.
static_assert(sizeof(dxfg_v4_message_connector_io_stats_t) == 12 * sizeof(std::int64_t),
              "dxfg_v4_message_connector_io_stats_t changed: update MessageConnectorInfo::IoStatistics");

MessageConnectorInfo getInfo(/* dxfg_v4_message_connector_t * */ void *connector) {
    if (connector == nullptr) {
        throw InvalidArgumentException("Unable to read the message connector. The `connector` handle is nullptr");
    }

    // The last error is read with an experimental function.
    Isolate::checkAdvancedRevision();

    auto *graalConnector = static_cast<dxfg_v4_message_connector_t *>(connector);
    MessageConnectorInfo info{};

    info.name = takeString(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getName, graalConnector));
    info.kind = graalKindToKind(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getKind, graalConnector));
    info.address = takeString(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getAddress, graalConnector));
    info.currentAddresses =
        takeStringList(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getCurrentAddresses, graalConnector));
    info.state = graalStateToState(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getState, graalConnector));
    info.active = runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_isActive, graalConnector) != 0;
    info.connectionCount = runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getConnectionCount, graalConnector);
    info.closedConnectionCount =
        runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getClosedConnectionCount, graalConnector);

    dxfg_v4_message_connector_stats_t stats{};

    runGraalV4Function(dxfg_v4_MessageConnector_getStatistics, graalConnector, &stats);

    info.statistics.readBytes = stats.read_bytes;
    info.statistics.writtenBytes = stats.written_bytes;
    info.statistics.readDataRecords = stats.read_data_records;
    info.statistics.writtenDataRecords = stats.written_data_records;
    info.statistics.activeConnectionCount = stats.active_connection_count;
    info.statistics.closedConnectionCount = stats.closed_connection_count;

    // Defensive: the minimum version of the SDK supports the feature.
    if (runGraalV4FunctionWithResult(dxfg_v4_system_is_feature_supported,
                                     DXFG_V4_FEATURE_MESSAGE_CONNECTOR_IO_STATISTICS) != 0) {
        dxfg_v4_message_connector_io_stats_t ioStats{};

        runGraalV4Function(dxfg_v4_MessageConnector_getIoStatistics, graalConnector, &ioStats);

        // Not ioStatistics.emplace(): clang rejects it, as a nested struct with default member initializers is not
        // default constructible for std::optional declared inside the enclosing class.
        MessageConnectorInfo::IoStatistics io{};

        io.readDataRecords = ioStats.read_data_records;
        io.writtenDataRecords = ioStats.written_data_records;
        io.readSubRecords = ioStats.read_sub_records;
        io.writtenSubRecords = ioStats.written_sub_records;
        io.readBytes = ioStats.read_bytes;
        io.writtenBytes = ioStats.written_bytes;
        io.readDataLagSumMicros = ioStats.read_data_lag_sum_us;
        io.writtenDataLagSumMicros = ioStats.written_data_lag_sum_us;
        io.readRttSumMicros = ioStats.read_rtt_sum_us;
        io.writtenRttSumMicros = ioStats.written_rtt_sum_us;
        io.activeConnectionCount = ioStats.active_connection_count;
        io.closedConnectionCount = ioStats.closed_connection_count;
        info.ioStatistics = io;
    }

    info.lastError = takeError(runGraalV4FunctionWithResult(dxfg_v4_MessageConnector_getLastError, graalConnector));

    return info;
}

std::vector<MessageConnectorInfo> getConnectors(const JavaObjectHandle<DXEndpoint> &endpoint) {
    auto *graalEndpoint = toGraal(endpoint, "dxfg_v4_DXEndpoint_getConnectors");

    Isolate::checkAdvancedRevision();

    auto *list = runGraalV4FunctionWithNonNullResult(dxfg_v4_DXEndpoint_getConnectors, graalEndpoint);

    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_message_connector_list_release, list);
    });

    std::vector<MessageConnectorInfo> result{};

    if (list->size > 0) {
        result.reserve(static_cast<std::size_t>(list->size));
    }

    for (std::int32_t i = 0; i < list->size; i++) {
        result.emplace_back(getInfo(list->elements[i]));
    }

    return result;
}

std::optional<ConnectionError> getLastConnectionError(const JavaObjectHandle<DXEndpoint> &endpoint) {
    auto *graalEndpoint = toGraal(endpoint, "dxfg_v4_DXEndpoint_getLastConnectionError");

    Isolate::checkAdvancedRevision();

    return takeError(runGraalV4FunctionWithResult(dxfg_v4_DXEndpoint_getLastConnectionError, graalEndpoint));
}

namespace Listener {

JavaObjectHandle<DXEndpointMessageConnectorListener> create(void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_MessageConnectorListener_new`. The `userFunc` parameter is nullptr");
    }

    Isolate::checkAdvancedRevision();

    return JavaObjectHandle<DXEndpointMessageConnectorListener>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_MessageConnectorListener_new, dxfcpp::bit_cast<dxfg_v4_message_connector_listener_func>(userFunc),
        userData));
}

void add(const JavaObjectHandle<DXEndpoint> &endpoint,
         const JavaObjectHandle<DXEndpointMessageConnectorListener> &listener) {
    constexpr auto function = "dxfg_v4_DXEndpoint_addMessageConnectorListener";
    auto *graalEndpoint = toGraal(endpoint, function);
    auto *graalListener = toGraal(listener, function);

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_DXEndpoint_addMessageConnectorListener, graalEndpoint, graalListener);
}

void remove(const JavaObjectHandle<DXEndpoint> &endpoint,
            const JavaObjectHandle<DXEndpointMessageConnectorListener> &listener) {
    constexpr auto function = "dxfg_v4_DXEndpoint_removeMessageConnectorListener";
    auto *graalEndpoint = toGraal(endpoint, function);
    auto *graalListener = toGraal(listener, function);

    Isolate::checkAdvancedRevision();
    runGraalV4Function(dxfg_v4_DXEndpoint_removeMessageConnectorListener, graalEndpoint, graalListener);
}

} // namespace Listener

} // namespace isolated::api::IsolatedMessageConnector

DXFCPP_END_NAMESPACE
