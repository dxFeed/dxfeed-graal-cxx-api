// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "../Common.hpp"
#include "EntityManager.hpp"

namespace dxfcpp {

struct OnDemandService;

using OnDemandServiceManager = EntityManager<OnDemandService>;

} // namespace dxfcpp