// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/promise/Promises.hpp"

#include "../../include/dxfeed_graal_cpp_api/isolated/promise/IsolatedPromise.hpp"
#include "../../include/dxfeed_graal_cpp_api/promise/Promise.hpp"

#include <memory>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<Promise<void>> Promises::allOfImpl(const std::vector<void *> &handles) {
    return std::make_shared<Promise<void>>(isolated::promise::IsolatedPromise::allOf(handles));
}

DXFCPP_END_NAMESPACE