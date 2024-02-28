// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

namespace dxfcpp {

struct EventMapper;

// TODO: implement [EN-8235]

class DXFCPP_EXPORT Configuration {};

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()