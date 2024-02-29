// Copyright (c) 2024 Devexperts LLC.
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

DXFCPP_BEGIN_NAMESPACE

static dxfcpp::InstrumentProfileConnection::State graalStateToState(dxfg_ipf_connection_state_t state) {
    switch (state) {
    case DXFG_IPF_CONNECTION_STATE_NOT_CONNECTED:
        return dxfcpp::InstrumentProfileConnection::State::NOT_CONNECTED;
    case DXFG_IPF_CONNECTION_STATE_CONNECTING:
        return dxfcpp::InstrumentProfileConnection::State::CONNECTING;
    case DXFG_IPF_CONNECTION_STATE_CONNECTED:
        return dxfcpp::InstrumentProfileConnection::State::CONNECTED;
    case DXFG_IPF_CONNECTION_STATE_COMPLETED:
        return dxfcpp::InstrumentProfileConnection::State::COMPLETED;
    case DXFG_IPF_CONNECTION_STATE_CLOSED:
        return dxfcpp::InstrumentProfileConnection::State::CLOSED;
    }

    return dxfcpp::InstrumentProfileConnection::State::NOT_CONNECTED;
}

struct InstrumentProfileConnection::Impl {
    static void onStateChange(graal_isolatethread_t * /*thread*/, dxfg_ipf_connection_state_t oldState,
                              dxfg_ipf_connection_state_t newState, void *userData) noexcept {
        auto id = Id<InstrumentProfileConnection>::from(
            dxfcpp::bit_cast<Id<InstrumentProfileConnection>::ValueType>(userData));
        auto connection = ApiContext::getInstance()->getManager<InstrumentProfileConnectionManager>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            Debugger::debug("InstrumentProfileConnection::Impl::onStateChange: id = " + std::to_string(id.getValue()));
        }

        if (connection) {
            connection->onStateChange_(graalStateToState(oldState), graalStateToState(newState));

            if (newState == DXFG_IPF_CONNECTION_STATE_CLOSED) {
                ApiContext::getInstance()->getManager<InstrumentProfileConnectionManager>()->unregisterEntity(id);
            }
        }
    }
};

InstrumentProfileConnection::InstrumentProfileConnection() noexcept
    : id_{Id<InstrumentProfileConnection>::UNKNOWN}, handle_{}, stateChangeListenerHandle_{}, onStateChange_{} {
}

InstrumentProfileConnection::Ptr
InstrumentProfileConnection::createConnection(const std::string &address,
                                              InstrumentProfileCollector::Ptr collector) noexcept {
    std::shared_ptr<InstrumentProfileConnection> connection(new (std::nothrow) InstrumentProfileConnection{});

    if (!connection || !collector->handle_) {
        // TODO: dummy connection & error handling [EN-8232];

        return connection;
    }

    connection->id_ =
        ApiContext::getInstance()->getManager<InstrumentProfileConnectionManager>()->registerEntity(connection);
    connection->handle_ = JavaObjectHandle<InstrumentProfileConnection>(
        isolated::ipf::InstrumentProfileConnection::createConnection(address, collector->handle_.get()));

    connection->stateChangeListenerHandle_ =
        JavaObjectHandle<IpfPropertyChangeListener>(isolated::ipf::IpfPropertyChangeListener::create(
            dxfcpp::bit_cast<void *>(&InstrumentProfileConnection::Impl::onStateChange),
            dxfcpp::bit_cast<void *>(connection->id_.getValue())));

    if (!connection->handle_ || !connection->stateChangeListenerHandle_) {
        return connection;
    }

    isolated::ipf::InstrumentProfileConnection::addStateChangeListener(connection->handle_.get(),
                                                                       connection->stateChangeListenerHandle_.get());

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

void InstrumentProfileConnection::close() const noexcept {
    if (!handle_) {
        return;
    }

    isolated::ipf::InstrumentProfileConnection::close(handle_.get());
}

bool InstrumentProfileConnection::waitUntilCompleted(std::int64_t timeout) const noexcept {
    if (!handle_) {
        return false;
    }

    return isolated::ipf::InstrumentProfileConnection::waitUntilCompleted(handle_.get(), timeout);
}

DXFCPP_END_NAMESPACE