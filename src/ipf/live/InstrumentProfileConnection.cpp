// Copyright (c) 2025 Devexperts LLC.
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

dxfcpp::InstrumentProfileConnection::State graalIpfConnectionStateToState(dxfg_ipf_connection_state_t state) {
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
        auto connection =
            ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileConnection>>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            Debugger::debug("InstrumentProfileConnection::Impl::onStateChange: id = " + std::to_string(id.getValue()));
        }

        if (connection) {
            connection->onStateChange_(graalIpfConnectionStateToState(oldState),
                                       graalIpfConnectionStateToState(newState));

            if (newState == DXFG_IPF_CONNECTION_STATE_CLOSED) {
                ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileConnection>>()->unregisterEntity(
                    id);
            }
        }
    }
};

InstrumentProfileConnection::InstrumentProfileConnection() noexcept
    : id_{Id<InstrumentProfileConnection>::UNKNOWN}, handle_{}, stateChangeListenerHandle_{}, onStateChange_{} {
}

InstrumentProfileConnection::Ptr
InstrumentProfileConnection::createConnection(const StringLike &address,
                                              InstrumentProfileCollector::Ptr collector) {
    std::shared_ptr<InstrumentProfileConnection> connection(new InstrumentProfileConnection{});

    connection->id_ =
        ApiContext::getInstance()->getManager<EntityManager<InstrumentProfileConnection>>()->registerEntity(connection);
    connection->handle_ =
        isolated::ipf::live::IsolatedInstrumentProfileConnection::createConnection(address, collector->handle_);
    connection->stateChangeListenerHandle_ = isolated::ipf::live::IsolatedIpfPropertyChangeListener::create(
        dxfcpp::bit_cast<void *>(&InstrumentProfileConnection::Impl::onStateChange),
        dxfcpp::bit_cast<void *>(connection->id_.getValue()));

    isolated::ipf::live::IsolatedInstrumentProfileConnection::addStateChangeListener(
        connection->handle_, connection->stateChangeListenerHandle_);

    return connection;
}

std::string InstrumentProfileConnection::getAddress() const {
    return isolated::ipf::live::IsolatedInstrumentProfileConnection::getAddress(handle_);
}

std::int64_t InstrumentProfileConnection::getUpdatePeriod() const {
    return isolated::ipf::live::IsolatedInstrumentProfileConnection::getUpdatePeriod(handle_);
}

void InstrumentProfileConnection::setUpdatePeriod(std::int64_t updatePeriod) const {
    isolated::ipf::live::IsolatedInstrumentProfileConnection::setUpdatePeriod(handle_, updatePeriod);
}

InstrumentProfileConnection::State InstrumentProfileConnection::getState() const {
    return isolated::ipf::live::IsolatedInstrumentProfileConnection::getState(handle_);
}

std::int64_t InstrumentProfileConnection::getLastModified() const {
    return isolated::ipf::live::IsolatedInstrumentProfileConnection::getLastModified(handle_);
}

void InstrumentProfileConnection::start() const {
    isolated::ipf::live::IsolatedInstrumentProfileConnection::start(handle_);
}

void InstrumentProfileConnection::close() const {
    isolated::ipf::live::IsolatedInstrumentProfileConnection::close(handle_);
}

bool InstrumentProfileConnection::waitUntilCompleted(std::int64_t timeout) const {
    return isolated::ipf::live::IsolatedInstrumentProfileConnection::waitUntilCompleted(handle_, timeout);
}

DXFCPP_END_NAMESPACE