// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXEndpoint {

void connect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint, const std::string &address) {
    if (!endpoint) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_DXEndpoint_connect`. The `endpoint` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_connect, static_cast<dxfg_endpoint_t *>(endpoint.get()),
                                           address.c_str());
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

} // namespace isolated::api::IsolatedDXEndpoint

DXFCPP_END_NAMESPACE