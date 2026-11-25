// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/IcebergType.hpp"

#include <array>
#include <functional>
#include <span>

DXFCPP_BEGIN_NAMESPACE

constinit const IcebergType IcebergType::UNDEFINED{0, "UNDEFINED"};
constinit const IcebergType IcebergType::NATIVE{1, "NATIVE"};
constinit const IcebergType IcebergType::SYNTHETIC{2, "SYNTHETIC"};

namespace {

constinit const std::array<std::reference_wrapper<const IcebergType>, 3> ALL_ELEMENTS{
    std::cref(IcebergType::UNDEFINED),
    std::cref(IcebergType::NATIVE),
    std::cref(IcebergType::SYNTHETIC),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const IcebergType>> IcebergType::ALL{ALL_ELEMENTS};

DXFCPP_END_NAMESPACE