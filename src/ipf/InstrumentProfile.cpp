// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

void InstrumentProfile::fillData(void *graalNative) noexcept {
}

void InstrumentProfile::fillGraalData(void *graalNative) const noexcept {
}

void InstrumentProfile::freeGraalData(void *graalNative) noexcept {
}

std::shared_ptr<InstrumentProfile> InstrumentProfile::fromGraal(void *graalNative) noexcept {
    return {};
}

void *InstrumentProfile::toGraal() const noexcept {
    return nullptr;
}

void InstrumentProfile::freeGraal(void *graalNative) noexcept {
}

} // namespace dxfcpp