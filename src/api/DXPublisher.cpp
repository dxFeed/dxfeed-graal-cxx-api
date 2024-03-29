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

std::shared_ptr<DXPublisher> DXPublisher::getInstance() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::getInstance()");
    }

    return DXEndpoint::getInstance()->getPublisher();
}

std::shared_ptr<DXPublisher> DXPublisher::create(void *handle) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXPublisher::create(" + dxfcpp::toString(handle) + ")");
    }

    std::shared_ptr<DXPublisher> publisher{new DXPublisher{}};

    auto id = ApiContext::getInstance()->getManager<DXPublisherManager>()->registerEntity(publisher);
    ignore_unused(id);

    publisher->handle_ = JavaObjectHandle<DXPublisher>(handle);

    return publisher;
}

std::string DXPublisher::toString() const noexcept {
    return fmt::format("DXPublisher{{{}}}", handle_.toString());
}

void DXPublisher::publishEventsImpl(void *graalEventsList) const noexcept {
    runIsolatedOrElse(
        [handle = static_cast<dxfg_publisher_t *>(handle_.get()), graalEventsList](auto threadHandle) {
            return dxfg_DXPublisher_publishEvents(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                  static_cast<dxfg_event_type_list *>(graalEventsList)) == 0;
        },
        false);
}

DXFCPP_END_NAMESPACE