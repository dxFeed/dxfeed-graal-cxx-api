// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <functional>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

const IcebergType IcebergType::UNDEFINED{0, "UNDEFINED"};
const IcebergType IcebergType::NATIVE{1, "NATIVE"};
const IcebergType IcebergType::SYNTHETIC{2, "SYNTHETIC"};

template <>
const std::unordered_map<IcebergType::CodeType, std::reference_wrapper<const IcebergType>> IcebergType::ParentType::ALL{
    {IcebergType::UNDEFINED.getCode(), std::cref(IcebergType::UNDEFINED)},
    {IcebergType::NATIVE.getCode(), std::cref(IcebergType::NATIVE)},
    {IcebergType::SYNTHETIC.getCode(), std::cref(IcebergType::SYNTHETIC)},
};

DXFCPP_END_NAMESPACE