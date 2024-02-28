// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

namespace dxfcpp {

namespace IsolatedString {

DXFCPP_EXPORT bool release(const char *string) noexcept;

}

}

DXFCXX_DISABLE_MSC_WARNINGS_POP()