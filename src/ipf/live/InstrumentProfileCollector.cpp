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

InstrumentProfileCollector::InstrumentProfileCollector() noexcept : id_{Id<InstrumentProfileCollector>::UNKNOWN}, handle_{} {
    handle_ = JavaObjectHandle<InstrumentProfileCollector>(dxfcpp::isolated::ipf::InstrumentProfileCollector::create());
}

InstrumentProfileCollector::Ptr InstrumentProfileCollector::create() noexcept {
    auto collector = std::shared_ptr<InstrumentProfileCollector>(new InstrumentProfileCollector());

    collector->id_ = ApiContext::getInstance()->getManager<InstrumentProfileCollectorManager>()->registerEntity(collector);

    return collector;
}

}