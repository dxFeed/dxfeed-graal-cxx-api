// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"
#include "InternalExport.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>
#include <thread>

DXFCPP_BEGIN_NAMESPACE

struct DXFCPP_INTERNAL_EXPORT Platform final {
    static std::size_t getLogicalCoresCount() noexcept;

    static std::string getPlatformInfo() noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()