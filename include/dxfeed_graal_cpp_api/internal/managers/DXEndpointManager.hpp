// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <unordered_map>

#include "../Common.hpp"

namespace dxfcpp {

struct DXEndpoint;

class DXEndpointManager {
    std::unordered_map<Id<DXEndpoint>, std::shared_ptr<DXEndpoint>> enities_;

};

} // namespace dxfcpp