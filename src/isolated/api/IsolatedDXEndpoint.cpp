// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXEndpoint.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXEndpoint {

void /* int32_t */
close(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_close`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_close, static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void /* int32_t */
closeAndAwaitTermination(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_closeAndAwaitTermination`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_closeAndAwaitTermination,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void /* int32_t */ user(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
                        std::string_view user) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_user`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_user, static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           user.data());
}

void /* int32_t */ password(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
                            std::string_view password) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_password`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_password, static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           password.data());
}

void connect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint, std::string_view address) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_connect`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_connect, static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           address.data());
}

void reconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_reconnect`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_reconnect, static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void disconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_disconnect`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_disconnect, static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void disconnectAndClear(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_disconnectAndClear`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_disconnectAndClear,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void awaitProcessed(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_awaitProcessed`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_awaitProcessed,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

void awaitNotConnected(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_awaitNotConnected`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_awaitNotConnected,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()));
}

dxfcpp::DXEndpoint::State getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument("Unable to get state. The `endpoint` handle is invalid");
    }

    return graalStateToState(runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_getState,
                                                                    static_cast<dxfg_endpoint_t *>(endpoint.get())));
}

void addStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
        &listener) {

    if (!endpoint) {
        throw std::invalid_argument("Unable to add DXEndpointStateChangeListener. The `endpoint` handle is invalid");
    }

    if (!listener) {
        throw std::invalid_argument("Unable to add DXEndpointStateChangeListener. The `listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_addStateChangeListener,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           static_cast<dxfg_endpoint_state_change_listener_t *>(listener.get()));
}

void removeStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
        &listener) {

    if (!endpoint) {
        throw std::invalid_argument("Unable to remove DXEndpointStateChangeListener. The `endpoint` handle is invalid");
    }

    if (!listener) {
        throw std::invalid_argument("Unable to remove DXEndpointStateChangeListener. The `listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_removeStateChangeListener,
                                           static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           static_cast<dxfg_endpoint_state_change_listener_t *>(listener.get()));
}

void * /* dxfg_feed_t* */ getFeed(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_getFeed`. The `endpoint` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_DXEndpoint_getFeed, static_cast<dxfg_endpoint_t *>(endpoint.get())));
}

void * /* dxfg_publisher_t* */
getPublisher(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_getPublisher`. The `endpoint` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(dxfg_DXEndpoint_getPublisher,
                                                                      static_cast<dxfg_endpoint_t *>(endpoint.get())));
}

/* dxfg_event_clazz_list_t* */ std::unordered_set<EventTypeEnum>
getEventTypes(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument("Unable to retrieve event types. The `endpoint` handle is invalid");
    }

    std::unordered_set<EventTypeEnum> result{};

    dxfg_event_clazz_list_t *eventTypesList = runGraalFunctionAndThrowIfNullptr(
        dxfg_DXEndpoint_getEventTypes, static_cast<dxfg_endpoint_t *>(endpoint.get()));

    if (eventTypesList->size > 0) {
        for (auto i = 0; i < eventTypesList->size; i++) {
            dxfg_event_clazz_t ec = *eventTypesList->elements[static_cast<std::size_t>(i)];

            if (auto etIt = EventTypeEnum::ALL_BY_ID.find(static_cast<std::uint32_t>(ec));
                etIt != EventTypeEnum::ALL_BY_ID.end()) {
                result.emplace(etIt->second.get());
            }
        }
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_EventClazz_release, eventTypesList);

    return result;
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
    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(dxfg_DXEndpoint_newBuilder));
}

void /* int32_t */
withRole(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
         /* dxfg_endpoint_role_t */ dxfcpp::DXEndpoint::Role role) {
    if (!builder) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_Builder_withRole`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_DXEndpoint_Builder_withRole, static_cast<dxfg_endpoint_builder_t *>(builder.get()), roleToGraalRole(role));
}

// dxfg_DXEndpoint_Builder_withProperty
void /* int32_t */
withProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
             std::string_view key, std::string_view value) {
    if (!builder) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_Builder_withProperty`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_Builder_withProperty,
                                           static_cast<dxfg_endpoint_builder_t *>(builder.get()), key.data(),
                                           value.data());
}

// dxfg_DXEndpoint_Builder_withProperties
void /* int32_t */
withProperties(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
               std::string_view filePath) {
    if (!builder) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_Builder_withProperties`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_Builder_withProperties,
                                           static_cast<dxfg_endpoint_builder_t *>(builder.get()), filePath.data());
}

// dxfg_DXEndpoint_Builder_supportsProperty
bool /* int32_t */
supportsProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
                 std::string_view key) {
    if (!builder) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_Builder_supportsProperty`. The `builder` handle is invalid");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_Builder_supportsProperty,
                                                  static_cast<dxfg_endpoint_builder_t *>(builder.get()),
                                                  key.data()) == 1;
}

// dxfg_DXEndpoint_Builder_build
void * /* dxfg_endpoint_t* */
build(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder) {
    if (!builder) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_Builder_build`. The `builder` handle is invalid");
    }

    return dxfcpp::bit_cast<void *>(runGraalFunctionAndThrowIfNullptr(
        dxfg_DXEndpoint_Builder_build, static_cast<dxfg_endpoint_builder_t *>(builder.get())));
}

} // namespace Builder

namespace StateChangeListener {
JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener> create(void *userFunc, void *userData) {
    if (!userFunc) {
        throw std::invalid_argument(
            "Unable to create DXEndpointStateChangeListener. The `userFunc` parameter is nullptr");
    }

    return JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>(runGraalFunctionAndThrowIfNullptr(
        dxfg_PropertyChangeListener_new, dxfcpp::bit_cast<dxfg_endpoint_state_change_listener_func>(userFunc),
        userData));
}
} // namespace StateChangeListener

} // namespace isolated::api::IsolatedDXEndpoint

DXFCPP_END_NAMESPACE