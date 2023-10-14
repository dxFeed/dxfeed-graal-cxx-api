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

InstrumentProfileConnection::InstrumentProfileConnection() noexcept
    : id_{Id<InstrumentProfileConnection>::UNKNOWN}, handle_{} {
}

InstrumentProfileConnection::Ptr
InstrumentProfileConnection::createConnection(const std::string &address,
                                              InstrumentProfileCollector::Ptr collector) noexcept {
    std::shared_ptr<InstrumentProfileConnection> connection{new (std::nothrow) InstrumentProfileConnection{}};

    if (!connection || !collector->handle_) {
        // TODO: dummy connection & error handling;

        return connection;
    }

    connection->id_ =
        ApiContext::getInstance()->getManager<InstrumentProfileConnectionManager>()->registerEntity(connection);
    connection->handle_ = JavaObjectHandle<InstrumentProfileConnection>(
        isolated::ipf::InstrumentProfileConnection::createConnection(address, collector->handle_.get()));

    return connection;
}

std::string InstrumentProfileConnection::getAddress() const noexcept {
    if (!handle_) {
        return String::EMPTY;
    }

    return isolated::ipf::InstrumentProfileConnection::getAddress(handle_.get());
}

std::int64_t InstrumentProfileConnection::getUpdatePeriod() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::ipf::InstrumentProfileConnection::getUpdatePeriod(handle_.get());
}

void InstrumentProfileConnection::setUpdatePeriod(std::int64_t updatePeriod) const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ipf::InstrumentProfileConnection::setUpdatePeriod(handle_.get(), updatePeriod);
}

InstrumentProfileConnection::State InstrumentProfileConnection::getState() const noexcept {
    if (!handle_) {
        return InstrumentProfileConnection::State::CLOSED;
    }

    return isolated::ipf::InstrumentProfileConnection::getState(handle_.get());
}

std::int64_t InstrumentProfileConnection::getLastModified() const noexcept {
    if (!handle_) {
        return 0;
    }

    return isolated::ipf::InstrumentProfileConnection::getLastModified(handle_.get());
}

void InstrumentProfileConnection::start() const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ipf::InstrumentProfileConnection::start(handle_.get());
}

} // namespace dxfcpp