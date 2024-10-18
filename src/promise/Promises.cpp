// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

std::shared_ptr<Promise<void>> Promises::allOfImpl(const std::vector<void*>& handles) {
    return std::make_shared<Promise<void>>(isolated::promise::IsolatedPromise::allOf(handles));
}

DXFCPP_END_NAMESPACE