// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "../Common.hpp"
#include "EntityManager.hpp"

namespace dxfcpp {

struct DXEndpoint;

using DXEndpointManager = EntityManager<DXEndpoint>;

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()