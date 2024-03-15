// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXEndpoint.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api {

namespace IsolatedDXEndpoint {

// dxfg_endpoint_t*                dxfg_DXEndpoint_getInstance(graal_isolatethread_t *thread);
// dxfg_endpoint_t*                dxfg_DXEndpoint_getInstance2(graal_isolatethread_t *thread, dxfg_endpoint_role_t
// role); dxfg_endpoint_t*                dxfg_DXEndpoint_create(graal_isolatethread_t *thread); dxfg_endpoint_t*
// dxfg_DXEndpoint_create2(graal_isolatethread_t *thread, dxfg_endpoint_role_t role); int32_t
// dxfg_DXEndpoint_close(graal_isolatethread_t *thread, dxfg_endpoint_t *endpoint); int32_t
// dxfg_DXEndpoint_closeAndAwaitTermination(graal_isolatethread_t *thread, dxfg_endpoint_t *endpoint);
// dxfg_endpoint_role_t            dxfg_DXEndpoint_getRole(graal_isolatethread_t *thread, dxfg_endpoint_t *endpoint);
// int32_t                         dxfg_DXEndpoint_user(graal_isolatethread_t *thread, dxfg_endpoint_t *endpoint, const
// char *user); int32_t                         dxfg_DXEndpoint_password(graal_isolatethread_t *thread, dxfg_endpoint_t
// *endpoint, const char *password);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_connect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void connect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint, const std::string &address);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_reconnect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void reconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_disconnect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void disconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_disconnectAndClear` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void disconnectAndClear(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_awaitProcessed` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void awaitProcessed(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_awaitNotConnected` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void awaitNotConnected(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function in isolation to get the state of the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @return The endpoint's state.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
dxfcpp::DXEndpoint::State getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function in isolation to add the state change listener to the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws std::invalid_argument if the endpoint handle or listener handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void addStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
        &listener);

/**
 * Calls the Graal SDK function in isolation to remove the state change listener to the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws std::invalid_argument if the endpoint handle or listener is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void removeStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
        &listener);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_getFeed` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_feed_t* */ getFeed(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_getPublisher` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void* /* dxfg_publisher_t* */ getPublisher(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);


// int32_t                         dxfg_DXEndpoint_executor(graal_isolatethread_t *thread, dxfg_endpoint_t
// *endpoint, dxfg_executor_t *executor);

/**
 * Calls the Graal SDK function in isolation to retrieve event types from the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @return A set of event types.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_event_clazz_list_t* */ std::unordered_set<EventTypeEnum>
getEventTypes(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

} // namespace IsolatedDXEndpoint

} // namespace isolated::api

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()