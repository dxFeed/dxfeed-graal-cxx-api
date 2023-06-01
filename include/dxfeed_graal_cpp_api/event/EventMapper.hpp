// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include <memory>
#include <vector>

#include "EventType.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT EventMapper {
    static std::vector<std::shared_ptr<EventType>> fromGraalNativeList(void *graalNativeList);
};

} // namespace dxfcpp