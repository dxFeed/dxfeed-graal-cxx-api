// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedDXEndpoint.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"

#include <dxfg_api.h>

#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXEndpoint {

namespace {

dxfg_v4_endpoint_t *toGraal(const JavaObjectHandle<DXEndpoint> &endpoint, const char *function) {
    if (!endpoint) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `endpoint` handle is invalid");
    }

    return static_cast<dxfg_v4_endpoint_t *>(endpoint.get());
}

dxfg_v4_endpoint_builder_t *toGraal(const JavaObjectHandle<DXEndpoint::Builder> &builder, const char *function) {
    if (!builder) {
        throw InvalidArgumentException(std::string("Unable to execute function `") + function +
                                       "`. The `builder` handle is invalid");
    }

    return static_cast<dxfg_v4_endpoint_builder_t *>(builder.get());
}

} // namespace

void /* int32_t */
close(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_close, toGraal(endpoint, "dxfg_v4_DXEndpoint_close"));
}

void /* int32_t */
closeAndAwaitTermination(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_closeAndAwaitTermination,
                       toGraal(endpoint, "dxfg_v4_DXEndpoint_closeAndAwaitTermination"));
}

void /* int32_t */ user(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint, std::string_view user) {
    runGraalV4Function(dxfg_v4_DXEndpoint_user, toGraal(endpoint, "dxfg_v4_DXEndpoint_user"),
                       SdkString{user, StringLabel::secretOf("DXEndpoint::user(user)")}.c_str());
}

void /* int32_t */ password(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint,
                            std::string_view password) {
    runGraalV4Function(dxfg_v4_DXEndpoint_password, toGraal(endpoint, "dxfg_v4_DXEndpoint_password"),
                       SdkString{password, StringLabel::secretOf("DXEndpoint::password(password)")}.c_str());
}

void connect(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint, std::string_view address) {
    runGraalV4Function(dxfg_v4_DXEndpoint_connect, toGraal(endpoint, "dxfg_v4_DXEndpoint_connect"),
                       SdkString{address, "DXEndpoint::connect(address)"}.c_str());
}

void reconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_reconnect, toGraal(endpoint, "dxfg_v4_DXEndpoint_reconnect"));
}

void disconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_disconnect, toGraal(endpoint, "dxfg_v4_DXEndpoint_disconnect"));
}

void disconnectAndClear(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_disconnectAndClear,
                       toGraal(endpoint, "dxfg_v4_DXEndpoint_disconnectAndClear"));
}

void awaitProcessed(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_awaitProcessed, toGraal(endpoint, "dxfg_v4_DXEndpoint_awaitProcessed"));
}

void awaitNotConnected(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    runGraalV4Function(dxfg_v4_DXEndpoint_awaitNotConnected,
                       toGraal(endpoint, "dxfg_v4_DXEndpoint_awaitNotConnected"));
}

DXEndpoint::State getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    return graalStateToState(runGraalV4FunctionWithResult(dxfg_v4_DXEndpoint_getState,
                                                          toGraal(endpoint, "dxfg_v4_DXEndpoint_getState")));
}

void addStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<DXEndpointStateChangeListener>
        &listener) {

    auto *graalEndpoint = toGraal(endpoint, "dxfg_v4_DXEndpoint_addStateChangeListener");

    if (!listener) {
        throw InvalidArgumentException("Unable to add DXEndpointStateChangeListener. The `listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXEndpoint_addStateChangeListener, graalEndpoint,
                       static_cast<dxfg_v4_endpoint_state_change_listener_t *>(listener.get()));
}

void removeStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<DXEndpointStateChangeListener>
        &listener) {

    auto *graalEndpoint = toGraal(endpoint, "dxfg_v4_DXEndpoint_removeStateChangeListener");

    if (!listener) {
        throw InvalidArgumentException(
            "Unable to remove DXEndpointStateChangeListener. The `listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXEndpoint_removeStateChangeListener, graalEndpoint,
                       static_cast<dxfg_v4_endpoint_state_change_listener_t *>(listener.get()));
}

void * /* dxfg_feed_t* */ getFeed(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXEndpoint_getFeed, toGraal(endpoint, "dxfg_v4_DXEndpoint_getFeed")));
}

void * /* dxfg_publisher_t* */
getPublisher(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXEndpoint_getPublisher, toGraal(endpoint, "dxfg_v4_DXEndpoint_getPublisher")));
}

/* dxfg_event_clazz_list_t* */ std::unordered_set<EventTypeEnum>
getEventTypes(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    return event::IsolatedEventClazzList::takeEventTypes(
        runGraalV4FunctionWithNonNullResult(dxfg_v4_DXEndpoint_getEventTypes,
                                            toGraal(endpoint, "dxfg_v4_DXEndpoint_getEventTypes")),
        "DXEndpoint");
}

std::int64_t getRequestedAggregationPeriodMillis(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    return runGraalV4FunctionWithResult(dxfg_v4_DXEndpoint_getRequestedAggregationPeriodMillis,
                                        toGraal(endpoint, "dxfg_v4_DXEndpoint_getRequestedAggregationPeriodMillis"));
}

void setRequestedAggregationPeriodMillis(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint,
                                         std::int64_t period) {
    runGraalV4Function(dxfg_v4_DXEndpoint_setRequestedAggregationPeriodMillis,
                       toGraal(endpoint, "dxfg_v4_DXEndpoint_setRequestedAggregationPeriodMillis"), period);
}

std::pair<std::int64_t, std::int64_t>
getAggregationPeriodInfo(/* dxfg_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    auto *graalEndpoint = toGraal(endpoint, "dxfg_v4_DXEndpoint_getAggregationPeriodInfo");

    // The helpers take one out-parameter, so the pair is read by hand.
    std::int64_t min{};
    std::int64_t max{};

    runGraalV4Function(dxfg_v4_DXEndpoint_getAggregationPeriodInfo, graalEndpoint, &min, &max);

    return {min, max};
}

dxfg_endpoint_role_t roleToGraalRole(DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFG_ENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFG_ENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFG_ENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFG_ENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFG_ENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFG_ENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFG_ENDPOINT_ROLE_FEED;
}

namespace Builder {

void * /* dxfg_endpoint_builder_t* */ create() {
    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(dxfg_v4_DXEndpoint_newBuilder));
}

void /* int32_t */
withRole(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder,
         /* dxfg_endpoint_role_t */ DXEndpoint::Role role) {
    runGraalV4Function(dxfg_v4_DXEndpoint_Builder_withRole, toGraal(builder, "dxfg_v4_DXEndpoint_Builder_withRole"),
                       roleToGraalRole(role));
}

void /* int32_t */
withProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder, std::string_view key,
             std::string_view value) {
    runGraalV4Function(dxfg_v4_DXEndpoint_Builder_withProperty,
                       toGraal(builder, "dxfg_v4_DXEndpoint_Builder_withProperty"),
                       SdkString{key, "DXEndpoint::Builder::withProperty(key)"}.c_str(),
                       SdkString{value, StringLabel::ofPropertyValue("DXEndpoint::Builder::withProperty(value)", key)}
                           .c_str());
}

void /* int32_t */
withProperties(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder,
               std::string_view filePath) {
    runGraalV4Function(dxfg_v4_DXEndpoint_Builder_withProperties,
                       toGraal(builder, "dxfg_v4_DXEndpoint_Builder_withProperties"),
                       SdkString{filePath, "DXEndpoint::Builder::withProperties(filePath)"}.c_str());
}

bool /* int32_t */
supportsProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder,
                 std::string_view key) {
    return runGraalV4FunctionWithResult(dxfg_v4_DXEndpoint_Builder_supportsProperty,
                                        toGraal(builder, "dxfg_v4_DXEndpoint_Builder_supportsProperty"),
                                        SdkString{key, "DXEndpoint::Builder::supportsProperty(key)"}.c_str()) == 1;
}

void /* int32_t */
withExecutor(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder,
             /* dxfg_executor_t * */ const JavaObjectHandle<ExecutorTag> &executor) {
    auto *graalBuilder = toGraal(builder, "dxfg_v4_DXEndpoint_Builder_withExecutor");

    if (!executor) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_DXEndpoint_Builder_withExecutor`. The `executor` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_DXEndpoint_Builder_withExecutor, graalBuilder,
                       static_cast<dxfg_v4_executor_t *>(executor.get()));
}

void * /* dxfg_endpoint_t* */
build(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<DXEndpoint::Builder> &builder) {
    return dxfcpp::bit_cast<void *>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_DXEndpoint_Builder_build, toGraal(builder, "dxfg_v4_DXEndpoint_Builder_build")));
}

} // namespace Builder

namespace StateChangeListener {
JavaObjectHandle<DXEndpointStateChangeListener> create(void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException(
            "Unable to create DXEndpointStateChangeListener. The `userFunc` parameter is nullptr");
    }

    return JavaObjectHandle<DXEndpointStateChangeListener>(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_PropertyChangeListener_new, dxfcpp::bit_cast<dxfg_v4_endpoint_state_change_listener_func>(userFunc),
        userData));
}
} // namespace StateChangeListener

} // namespace isolated::api::IsolatedDXEndpoint

DXFCPP_END_NAMESPACE
