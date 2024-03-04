// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../api/DXEndpoint.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api {

namespace IsolatedDXEndpoint {

// Throws:
//   std::invalid_argument if endpoint handle is invalid.
//   JavaException if something happened with the dxFeed API backend
//   GraalException if something happened with the GraalVM
static dxfcpp::DXEndpoint::State
getState(/* dxfg_endpoint_t* */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint);

}

}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()