// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

namespace dxfcpp {

/// Base abstract class for all dxFeed C++ API entities
struct DXFCPP_EXPORT Entity {
    /// The default virtual d-tor
    virtual ~Entity() noexcept = default;
};

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()