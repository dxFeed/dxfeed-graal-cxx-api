// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <optional>
#include <string>

#include <dxfeed_graal_cpp_api/detail/StringParam.hpp>

namespace {

void take(dxfcpp::StringParam) {
}

} // namespace

void use() {
    take(std::optional<std::string>{"abc"});
}
