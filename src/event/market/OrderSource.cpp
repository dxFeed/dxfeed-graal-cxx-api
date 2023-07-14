// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void *OrderSource::toGraal() const noexcept {
    auto *graalSource = new (std::nothrow)
        dxfg_indexed_event_source_t{ORDER_SOURCE, static_cast<std::int32_t>(id()), createCString(name())};

    return dxfcpp::bit_cast<void *>(graalSource);
}

} // namespace dxfcpp