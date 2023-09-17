// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Common.hpp"
#include "Conf.hpp"

#include <string>
#include <thread>

namespace dxfcpp {
struct DXFCPP_EXPORT Console final {
    static std::pair<std::size_t, size_t> getSize() noexcept;
};
} // namespace dxfcpp