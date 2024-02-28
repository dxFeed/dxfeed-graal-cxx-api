// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>
#include <thread>

namespace dxfcpp {
struct DXFCPP_EXPORT Platform final {
    static std::size_t getLogicalCoresCount() noexcept {
        return std::thread::hardware_concurrency();
    }

    static std::string getPlatformInfo() noexcept;
};
} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()