// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <vector>
#include <memory>

#include "EventType.hpp"

namespace dxfcpp {

struct EventMapper {
    static std::vector<std::shared_ptr<EventType>> fromGraalNativeList(void *graalNativeList);
};

}