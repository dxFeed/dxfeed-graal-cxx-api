// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXEndpoint.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXEndpoint {

// dxfg_endpoint_t*                dxfg_DXEndpoint_getInstance(graal_isolatethread_t *thread);
// dxfg_endpoint_t*                dxfg_DXEndpoint_getInstance2(graal_isolatethread_t *thread, dxfg_endpoint_role_t
// role);
//
// dxfg_endpoint_t*                dxfg_DXEndpoint_create(graal_isolatethread_t *thread);
//
// dxfg_endpoint_t* dxfg_DXEndpoint_create2(graal_isolatethread_t *thread, dxfg_endpoint_role_t role);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_close` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ close(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_closeAndAwaitTermination` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
closeAndAwaitTermination(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

// dxfg_endpoint_role_t            dxfg_DXEndpoint_getRole(graal_isolatethread_t *thread, dxfg_endpoint_t *endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_user` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @param user The user's name.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ user(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
                        std::string_view user);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_password` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @param password The user's password.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */ password(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
                            std::string_view password);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_connect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void connect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint, std::string_view address);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_reconnect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void reconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_disconnect` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void disconnect(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_disconnectAndClear` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void disconnectAndClear(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_awaitProcessed` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void awaitProcessed(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_awaitNotConnected` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void awaitNotConnected(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function in isolation to get the state of the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @return The endpoint's state.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
dxfcpp::DXEndpoint::State getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function in isolation to add the state change listener to the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws InvalidArgumentException if the endpoint handle or listener handle is invalid.
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
 * @throws InvalidArgumentException if the endpoint handle or listener is invalid.
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
 * @return dxFeed Graal SDK DXFeed's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_feed_t* */ getFeed(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_getPublisher` in isolation.
 *
 * @param endpoint The endpoint's handle.
 * @return dxFeed Graal SDK DXPublisher's handle.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_publisher_t* */
getPublisher(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

// int32_t                         dxfg_DXEndpoint_executor(graal_isolatethread_t *thread, dxfg_endpoint_t
// *endpoint, dxfg_executor_t *executor);

/**
 * Calls the Graal SDK function in isolation to retrieve event types from the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @return A set of event types.
 * @throws InvalidArgumentException if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_event_clazz_list_t* */ std::unordered_set<EventTypeEnum>
getEventTypes(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

namespace Builder {

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_newBuilder` in isolation.
 *
 * @return dxFeed Graal SDK DXEndpoint::Builder's handle.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_endpoint_builder_t* */ create();

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_Builder_withRole` in isolation.
 * @param builder The DXEndpoint::Builder's handle.
 * @param role The endpoint's role.
 * @throws InvalidArgumentException if DXEndpoint::Builder's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
withRole(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
         /* dxfg_endpoint_role_t */ dxfcpp::DXEndpoint::Role role);

// int32_t                     dxfg_DXEndpoint_Builder_withName(graal_isolatethread_t *thread, dxfg_endpoint_builder_t
// *builder, const char *name);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_Builder_withProperty` in isolation.
 * @param builder The DXEndpoint::Builder's handle.
 * @param key The endpoint's property key.
 * @param value The endpoint's property value.
 * @throws InvalidArgumentException if DXEndpoint::Builder's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
withProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
             std::string_view key, std::string_view value);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_Builder_withProperties` in isolation.
 * @param builder The DXEndpoint::Builder's handle.
 * @param filePath The file with properties.
 * @throws InvalidArgumentException if DXEndpoint::Builder's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void /* int32_t */
withProperties(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
               std::string_view filePath);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_Builder_supportsProperty` in isolation.
 * @param builder The DXEndpoint::Builder's handle.
 * @param key The endpoint's property key to check.
 * @throws InvalidArgumentException if DXEndpoint::Builder's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
bool /* int32_t */
supportsProperty(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder,
                 std::string_view key);

/**
 * Calls the Graal SDK function `dxfg_DXEndpoint_Builder_build` in isolation.
 * @param builder The DXEndpoint::Builder's handle.
 * @return The DXEndpoint's handle.
 *
 * @throws InvalidArgumentException if DXEndpoint::Builder's handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void * /* dxfg_endpoint_t* */
build(/* dxfg_endpoint_builder_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint::Builder> &builder);

} // namespace Builder

namespace StateChangeListener {

/**
 * Calls the Graal SDK function `dxfg_PropertyChangeListener_new` in isolation.
 * @param userFunc A user function that is used as a callback for the listener.
 * @param userData User data, which is placed each time as a callback parameter when called from listener.
 * @return The DXEndpointStateChangeListener's handle.
 *
 * @throws InvalidArgumentException if userFunc is nullptr.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
/* dxfg_endpoint_state_change_listener_t* */ JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
create(/* dxfg_endpoint_state_change_listener_func */ void *userFunc, void *userData);

} // namespace StateChangeListener

} // namespace isolated::api::IsolatedDXEndpoint

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()