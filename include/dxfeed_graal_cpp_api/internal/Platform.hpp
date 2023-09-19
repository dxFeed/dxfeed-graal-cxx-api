// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Common.hpp"
#include "Conf.hpp"

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