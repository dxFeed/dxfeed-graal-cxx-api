// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXEndpoint {
dxfcpp::DXEndpoint::State getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw std::invalid_argument("Unable to get state. The `endpoint` handle is invalid");
    }

    return graalStateToState(runGraalFunctionAndThrowIfLessThanZero(dxfg_DXEndpoint_getState,
                                                                    static_cast<dxfg_endpoint_t *>(endpoint.get())));
}
} // namespace isolated::api::IsolatedDXEndpoint

DXFCPP_END_NAMESPACE