// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXEndpoint.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api {

namespace IsolatedDXEndpoint {

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

// Throws:
//   std::invalid_argument if endpoint or listener handle is invalid.
//   JavaException if something happened with the dxFeed API backend
//   GraalException if something happened with the GraalVM

/**
 * Calls the Graal SDK function in isolation to add the state change listener to the endpoint.
 *
 * @param endpoint The endpoint's handle.
 * @param listener The listener's handle.
 * @throws std::invalid_argument if endpoint handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend.
 * @throws GraalException if something happened with the GraalVM.
 */
void
addStateChangeListener(
    /* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint,
    /* dxfg_endpoint_state_change_listener_t * */ const JavaObjectHandle<dxfcpp::DXEndpointStateChangeListener>
        &listener);

} // namespace IsolatedDXEndpoint

} // namespace isolated::api

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()