// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

/// Base abstract class for all dxFeed C++ API entities
struct DXFCPP_EXPORT Entity {
    /// The default virtual d-tor
    virtual ~Entity() noexcept = default;
};

} // namespace dxfcpp