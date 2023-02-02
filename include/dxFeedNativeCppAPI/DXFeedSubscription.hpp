// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>

namespace dxfcpp {

struct DXFeedSubscription : std::enable_shared_from_this<DXFeedSubscription> {
    virtual ~DXFeedSubscription() = default;
};

} // namespace dxfcpp