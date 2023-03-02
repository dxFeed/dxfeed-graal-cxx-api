// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace dxfcpp {

/// Base abstract class for all dxFeed C++ API entities
struct Entity {
    /// The default virtual d-tor
    virtual ~Entity() = default;
};

} // namespace dxfcpp