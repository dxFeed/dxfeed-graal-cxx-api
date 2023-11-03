// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

CEntryPointErrorsEnum Isolate::IsolateThread::detach() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::detach()");
    }

    // OK if nothing is attached.
    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detach(): !handle => Not attached");
        }

        return CEntryPointErrorsEnum::NO_ERROR;
    }

    auto result =
        static_cast<CEntryPointErrorsEnum>(graal_detach_thread(dxfcpp::bit_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrorsEnum::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detach(): result == CEntryPointErrorsEnum::NO_ERROR => Detached");
        }

        handle = nullptr;
    }

    return result;
}

CEntryPointErrorsEnum Isolate::IsolateThread::detachAllThreadsAndTearDownIsolate() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate()");
    }

    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate(): !handle => Not attached");
        }

        return CEntryPointErrorsEnum::NO_ERROR;
    }

    auto result = static_cast<CEntryPointErrorsEnum>(
        graal_detach_all_threads_and_tear_down_isolate(dxfcpp::bit_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrorsEnum::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() +
                            "::detachAllThreadsAndTearDownIsolate(): CEntryPointErrorsEnum::NO_ERROR => All "
                            "threads have been detached. The isolate has been teared down.");
        }

        handle = nullptr;
    }

    return result;
}

std::shared_ptr<Isolate> Isolate::create() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::create()");
    }

    graal_isolate_t *graalIsolateHandle{};
    graal_isolatethread_t *graalIsolateThreadHandle{};

    if (static_cast<CEntryPointErrorsEnum>(graal_create_isolate(
            nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) == CEntryPointErrorsEnum::NO_ERROR) {

        auto result =
            std::shared_ptr<Isolate>{new (std::nothrow) Isolate{graalIsolateHandle, graalIsolateThreadHandle}};

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace("Isolate::create() -> *" + result->toString());
        }

        return result;
    }

    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::create() -> nullptr");
    }

    return nullptr;
}

CEntryPointErrorsEnum Isolate::attach() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::attach()");
    }

    // We will not re-attach.
    if (!currentIsolateThread_.handle) {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): !currentIsolateThread_.handle => Needs to be attached.");
        }

        graal_isolatethread_t *newIsolateThreadHandle{};

        if (auto result = static_cast<CEntryPointErrorsEnum>(
                graal_attach_thread(dxfcpp::bit_cast<graal_isolate_t *>(handle_), &newIsolateThreadHandle));
            result != CEntryPointErrorsEnum::NO_ERROR) {

            if constexpr (Debugger::traceIsolates) {
                Debugger::trace(toString() + "::attach(): result != CEntryPointErrorsEnum::NO_ERROR [" +
                                std::to_string(static_cast<std::underlying_type_t<CEntryPointErrorsEnum>>(result)) +
                                "] " + CEntryPointErrors::valueOf(result).getDescription());
            }

            return result;
        }

        currentIsolateThread_.handle = newIsolateThreadHandle;
        currentIsolateThread_.isMain = mainIsolateThread_.handle == newIsolateThreadHandle;

        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): Attached: " + currentIsolateThread_.toString());
        }
    } else {
        if constexpr (Debugger::traceIsolates) {
            Debugger::trace(toString() + "::attach(): Cached: " + currentIsolateThread_.toString());
        }
    }

    return CEntryPointErrorsEnum::NO_ERROR;
}

GraalIsolateThreadHandle Isolate::get() noexcept {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace(toString() + "::get()");
    }

    return dxfcpp::bit_cast<void *>(graal_get_current_thread(dxfcpp::bit_cast<graal_isolate_t *>(handle_)));
}

} // namespace dxfcpp

// see isolated/isolated.hpp

namespace dxfcpp::isolated {

bool String::release(const char *string) noexcept {
    if (!string) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&string) {
            return dxfg_String_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), string) == 0;
        },
        false, string);
}

bool StringList::release(/* dxfg_string_list* */ void *stringList) noexcept {
    if (!stringList) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&stringList) {
            return dxfg_CList_String_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), stringList) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_string_list *>(stringList));
}

namespace api {

static dxfcpp::DXEndpoint::State graalStateToState(dxfg_endpoint_state_t state) {
    switch (state) {
    case DXFG_ENDPOINT_STATE_NOT_CONNECTED:
        return dxfcpp::DXEndpoint::State::NOT_CONNECTED;
    case DXFG_ENDPOINT_STATE_CONNECTING:
        return dxfcpp::DXEndpoint::State::CONNECTING;
    case DXFG_ENDPOINT_STATE_CONNECTED:
        return dxfcpp::DXEndpoint::State::CONNECTED;
    case DXFG_ENDPOINT_STATE_CLOSED:
        return dxfcpp::DXEndpoint::State::CLOSED;
    }

    return dxfcpp::DXEndpoint::State::NOT_CONNECTED;
}

bool DXEndpoint::close(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle) noexcept {
    if (!graalDXEndpointHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalDXEndpointHandle) {
            return dxfg_DXEndpoint_close(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                         graalDXEndpointHandle) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_endpoint_t *>(graalDXEndpointHandle));
}

bool DXEndpoint::user(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &user) noexcept {
    if (!graalDXEndpointHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalDXEndpointHandle, auto &&user) {
            return dxfg_DXEndpoint_user(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), graalDXEndpointHandle,
                                        user.c_str()) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_endpoint_t *>(graalDXEndpointHandle), user);
}

bool DXEndpoint::password(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &password) noexcept {
    if (!graalDXEndpointHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalDXEndpointHandle, auto &&password) {
            return dxfg_DXEndpoint_password(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                            graalDXEndpointHandle, password.c_str()) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_endpoint_t *>(graalDXEndpointHandle), password);
}

bool DXEndpoint::connect(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle, const std::string &address) noexcept {
    if (!graalDXEndpointHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalDXEndpointHandle, auto &&address) {
            return dxfg_DXEndpoint_connect(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                           graalDXEndpointHandle, address.c_str()) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_endpoint_t *>(graalDXEndpointHandle), address);
}

dxfcpp::DXEndpoint::State DXEndpoint::getState(/* dxfg_endpoint_t* */ void *graalDXEndpointHandle) noexcept {
    if (!graalDXEndpointHandle) {
        return dxfcpp::DXEndpoint::State::CLOSED;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalDXEndpointHandle) {
            return graalStateToState(dxfg_DXEndpoint_getState(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                              graalDXEndpointHandle));
        },
        dxfcpp::DXEndpoint::State::CLOSED, dxfcpp::bit_cast<dxfg_endpoint_t *>(graalDXEndpointHandle));
}

} // namespace api

namespace ipf {

/* dxfg_instrument_profile_reader_t* */ void *InstrumentProfileReader::create() noexcept {
    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle) {
            return dxfg_InstrumentProfileReader_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle));
        },
        nullptr));
}

std::int64_t InstrumentProfileReader::getLastModified(
    /* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept {
    if (!graalInstrumentProfileReaderHandle) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalInstrumentProfileReaderHandle) {
            return dxfg_InstrumentProfileReader_getLastModified(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                                graalInstrumentProfileReaderHandle);
        },
        0, dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(graalInstrumentProfileReaderHandle));
}

bool InstrumentProfileReader::wasComplete(
    /* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle) noexcept {
    if (!graalInstrumentProfileReaderHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&graalInstrumentProfileReaderHandle) {
            return dxfg_InstrumentProfileReader_wasComplete(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                            graalInstrumentProfileReaderHandle) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(graalInstrumentProfileReaderHandle));
}

/* dxfg_instrument_profile_list* */ void *
InstrumentProfileReader::readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                                      const std::string &address) noexcept {
    if (!graalInstrumentProfileReaderHandle) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&graalInstrumentProfileReaderHandle, auto &&address) {
            return dxfg_InstrumentProfileReader_readFromFile(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                             graalInstrumentProfileReaderHandle, address.c_str());
        },
        nullptr, dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(graalInstrumentProfileReaderHandle), address));
}

/* dxfg_instrument_profile_list* */ void *
InstrumentProfileReader::readFromFile(/* dxfg_instrument_profile_reader_t * */ void *graalInstrumentProfileReaderHandle,
                                      const std::string &address, const std::string &user,
                                      const std::string &password) noexcept {
    if (!graalInstrumentProfileReaderHandle) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&graalInstrumentProfileReaderHandle, auto &&address, auto &&user, auto &&password) {
            return dxfg_InstrumentProfileReader_readFromFile2(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                              graalInstrumentProfileReaderHandle, address.c_str(),
                                                              user.c_str(), password.c_str());
        },
        nullptr, dxfcpp::bit_cast<dxfg_instrument_profile_reader_t *>(graalInstrumentProfileReaderHandle), address,
        user, password));
}

std::string InstrumentProfileReader::resolveSourceURL(const std::string &address) noexcept {
    auto resolvedURL = runIsolatedOrElse(
        [](auto threadHandle, auto &&address) {
            return dxfg_InstrumentProfileReader_resolveSourceURL(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), address.c_str());
        },
        nullptr, address);

    auto result = dxfcpp::toString(resolvedURL);

    String::release(resolvedURL);

    return result;
}

/* dxfg_ipf_collector_t* */ void *InstrumentProfileCollector::create() noexcept {
    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle) {
            return dxfg_InstrumentProfileCollector_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle));
        },
        nullptr));
}

std::int64_t InstrumentProfileCollector::getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept {
    if (!instrumentProfileCollectorHandle) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileCollectorHandle) {
            return dxfg_InstrumentProfileCollector_getLastUpdateTime(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileCollectorHandle);
        },
        0, dxfcpp::bit_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle));
}

bool InstrumentProfileCollector::updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
    /* dxfg_instrument_profile_t * */ void *ip) noexcept {
    if (!instrumentProfileCollectorHandle || !ip) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileCollectorHandle, auto &&ip) {
            return dxfg_InstrumentProfileCollector_updateInstrumentProfile(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileCollectorHandle, ip) ==
                   0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle),
        dxfcpp::bit_cast<dxfg_instrument_profile_t *>(ip));
}

bool InstrumentProfileCollector::addUpdateListener(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
                                                   /* dxfg_ipf_update_listener_t* */ void *listener) noexcept {
    if (!instrumentProfileCollectorHandle || !listener) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileCollectorHandle, auto &&listener) {
            return dxfg_InstrumentProfileCollector_addUpdateListener(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileCollectorHandle,
                       listener) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle),
        dxfcpp::bit_cast<dxfg_ipf_update_listener_t *>(listener));
}

/* dxfg_ipf_connection_t* */ void *InstrumentProfileConnection::createConnection(
    const std::string &address,
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept {
    if (!instrumentProfileCollectorHandle) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&address, auto &&instrumentProfileCollectorHandle) {
            return dxfg_InstrumentProfileConnection_createConnection(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), address.c_str(),
                instrumentProfileCollectorHandle);
        },
        nullptr, address, dxfcpp::bit_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle)));
}

std::string InstrumentProfileConnection::getAddress(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return dxfcpp::String::EMPTY;
    }

    auto address = runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return dxfg_InstrumentProfileConnection_getAddress(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                               instrumentProfileConnectionHandle);
        },
        nullptr, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));

    auto result = dxfcpp::toString(address);

    String::release(address);

    return result;
}

std::int64_t InstrumentProfileConnection::getUpdatePeriod(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return dxfg_InstrumentProfileConnection_getUpdatePeriod(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle);
        },
        0, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::setUpdatePeriod(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
                                                  std::int64_t updatePeriod) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle, auto &&updatePeriod) {
            return dxfg_InstrumentProfileConnection_setUpdatePeriod(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle,
                       updatePeriod) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle), updatePeriod);
}

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

dxfcpp::InstrumentProfileConnection::State
InstrumentProfileConnection::getState(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return dxfcpp::InstrumentProfileConnection::State::CLOSED;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return graalStateToState(dxfg_InstrumentProfileConnection_getState(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle));
        },
        dxfcpp::InstrumentProfileConnection::State::CLOSED,
        dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

std::int64_t InstrumentProfileConnection::getLastModified(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return dxfg_InstrumentProfileConnection_getLastModified(
                dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle);
        },
        0, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::start(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return dxfg_InstrumentProfileConnection_start(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                          instrumentProfileConnectionHandle) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::close(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle) {
            return dxfg_InstrumentProfileConnection_close(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                          instrumentProfileConnectionHandle) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::addStateChangeListener(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
    /* dxfg_ipf_connection_state_change_listener_t * */ void *listener) noexcept {
    if (!instrumentProfileConnectionHandle || !listener) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle, auto &&listener) {
            return dxfg_InstrumentProfileConnection_addStateChangeListener(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle,
                       listener) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle),
        dxfcpp::bit_cast<dxfg_ipf_connection_state_change_listener_t *>(listener));
}

bool InstrumentProfileConnection::waitUntilCompleted(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle, std::int64_t timeoutInMs) noexcept {
    if (!instrumentProfileConnectionHandle) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle, auto &&listener) {
            return dxfg_InstrumentProfileConnection_waitUntilCompleted(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle,
                       listener) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle), timeoutInMs);
}

/* dxfg_ipf_connection_state_change_listener_t* */ void *
IpfPropertyChangeListener::create(/* dxfg_ipf_connection_state_change_listener_func */ void *userFunc,
                                  void *userData) noexcept {
    if (!userFunc) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&userFunc, auto &&userData) {
            return dxfg_IpfPropertyChangeListener_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), userFunc,
                                                      userData);
        },
        nullptr, dxfcpp::bit_cast<dxfg_ipf_connection_state_change_listener_func>(userFunc), userData));
}

bool InstrumentProfile::release(/* dxfg_instrument_profile_t* */ void *ip) noexcept {
    if (!ip) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&ip) {
            return dxfg_InstrumentProfile_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), ip) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_instrument_profile_t *>(ip));
}

bool InstrumentProfileList::release(/* dxfg_instrument_profile_list * */ void *graalInstrumentProfileList) noexcept {
    if (!graalInstrumentProfileList) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&list) {
            return dxfg_CList_InstrumentProfile_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                        list) == 0;
        },
        false, dxfcpp::bit_cast<dxfg_instrument_profile_list *>(graalInstrumentProfileList));
}

bool InstrumentProfileIterator::hasNext(/* dxfg_iterable_ip_t * */ void *iterable) noexcept {
    if (!iterable) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&iterable) {
            return dxfg_Iterable_InstrumentProfile_hasNext(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                           iterable) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_iterable_ip_t *>(iterable));
}

/* dxfg_instrument_profile_t* */ void *
InstrumentProfileIterator::next(/* dxfg_iterable_ip_t * */ void *iterable) noexcept {
    if (!iterable) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&iterable) {
            return dxfg_Iterable_InstrumentProfile_next(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                        iterable);
        },
        nullptr, dxfcpp::bit_cast<dxfg_iterable_ip_t *>(iterable)));
}

/* dxfg_ipf_update_listener_t* */ void *
InstrumentProfileUpdateListener::create(/* dxfg_ipf_update_listener_function */ void *userFunc,
                                        void *userData) noexcept {
    if (!userFunc) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&userFunc, auto &&userData) {
            return dxfg_InstrumentProfileUpdateListener_new(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                            userFunc, userData);
        },
        nullptr, dxfcpp::bit_cast<dxfg_ipf_update_listener_function>(userFunc), userData));
}

} // namespace ipf

namespace schedule {

/* dxfg_schedule_t* */ void *Day::getSchedule(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getSchedule(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        nullptr, static_cast<dxfg_day_t *>(day)));
}

std::int32_t Day::getDayId(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayId(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getYearMonthDay(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getYearMonthDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getYear(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getYear(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getMonthOfYear(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getMonthOfYear(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getDayOfMonth(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayOfMonth(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getDayOfWeek(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayOfWeek(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

bool Day::isHoliday(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isHoliday(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

bool Day::isShortDay(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isShortDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

bool Day::isTrading(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isTrading(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

std::int64_t Day::getStartTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getStartTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int64_t Day::getEndTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getEndTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

bool Day::containsTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept {
    if (!day) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&time) {
            return dxfg_Day_containsTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, time) == 1;
        },
        false, static_cast<dxfg_day_t *>(day), time);
}

std::int64_t Day::getResetTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getResetTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

/* dxfg_session_t* */ void *Day::getSessionByTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept {
    if (!day) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&time) {
            return dxfg_Day_getSessionByTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, time);
        },
        nullptr, static_cast<dxfg_day_t *>(day), time));
}

/* dxfg_session_t* */ void *Day::getFirstSession(/* dxfg_day_t* */ void *day,
                                                 /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getFirstSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::getLastSession(/* dxfg_day_t* */ void *day,
                                                /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getLastSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::findFirstSession(/* dxfg_day_t* */ void *day,
                                                  /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findFirstSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::findLastSession(/* dxfg_day_t* */ void *day,
                                                 /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findLastSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::getPrevDay(/* dxfg_day_t* */ void *day,
                                        /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getPrevDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::getNextDay(/* dxfg_day_t* */ void *day,
                                        /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getNextDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::findPrevDay(/* dxfg_day_t* */ void *day,
                                         /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findPrevDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::findNextDay(/* dxfg_day_t* */ void *day, /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findNextDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

std::size_t Day::getHashCode(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return dxfcpp::bit_cast<std::size_t>(day);
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_hashCode(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, dxfcpp::bit_cast<dxfg_day_t *>(day));
}

bool Day::equals(/* dxfg_day_t* */ void *day, /* dxfg_day_t* */ void *otherDay) noexcept {
    if (!day || !otherDay) {
        return false;
    }

    if (day == otherDay) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&otherDay) {
            return dxfg_Day_equals(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day, otherDay) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_day_t *>(day), dxfcpp::bit_cast<dxfg_day_t *>(otherDay));
}

std::string Day::toString(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_toString(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        nullptr, dxfcpp::bit_cast<dxfg_day_t *>(day));

    auto result = dxfcpp::toString(string);

    String::release(string);

    return result;
}

/* dxfg_session_filter_t* */ void *SessionFilter::getInstance(std::uint32_t code) noexcept {
    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&filter) {
            return dxfg_SessionFilter_getInstance(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), filter);
        },
        nullptr, static_cast<dxfg_session_filter_prepare_t>(code)));
}

/* dxfg_day_t* */ void *Session::getDay(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session)));
}

std::int32_t Session::getType(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getType(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

bool Session::isTrading(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_isTrading(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

bool Session::isEmpty(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_isEmpty(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

std::int64_t Session::getStartTime(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getStartTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

std::int64_t Session::getEndTime(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getEndTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

bool Session::containsTime(/* dxfg_session_t* */ void *session, std::int64_t time) noexcept {
    if (!session) {
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&time) {
            return dxfg_Session_containsTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session, time) ==
                   1;
        },
        false, dxfcpp::bit_cast<dxfg_session_t *>(session), time);
}

/* dxfg_session_t* */ void *Session::getPrevSession(/* dxfg_session_t* */ void *session,
                                                    /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_getPrevSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session,
                                               filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session), dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::getNextSession(/* dxfg_session_t* */ void *session,
                                                    /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_getNextSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session,
                                               filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session), dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::findPrevSession(/* dxfg_session_t* */ void *session,
                                                     /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_findPrevSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session,
                                                filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session), dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::findNextSession(/* dxfg_session_t* */ void *session,
                                                     /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_findNextSession(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session,
                                                filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session), dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

std::size_t Session::getHashCode(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return dxfcpp::bit_cast<std::size_t>(session);
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_hashCode(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, dxfcpp::bit_cast<dxfg_session_t *>(session));
}

bool Session::equals(/* dxfg_session_t* */ void *session, /* dxfg_session_t* */ void *otherSession) noexcept {
    if (!session || !otherSession) {
        return false;
    }

    if (session == otherSession) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&otherSession) {
            return dxfg_Session_equals(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session,
                                       otherSession) == 1;
        },
        false, dxfcpp::bit_cast<dxfg_session_t *>(session), dxfcpp::bit_cast<dxfg_session_t *>(otherSession));
}

std::string Session::toString(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_toString(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        nullptr, dxfcpp::bit_cast<dxfg_session_t *>(session));

    auto result = dxfcpp::toString(string);

    String::release(string);

    return result;
}

/* dxfg_day_filter_t* */ void *DayFilter::getInstance(std::uint32_t code) noexcept {
    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&filter) {
            return dxfg_DayFilter_getInstance(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), filter);
        },
        nullptr, static_cast<dxfg_day_filter_prepare_t>(code)));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept {
    if (!instrumentProfile) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile) {
            return dxfg_Schedule_getInstance(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                             instrumentProfile);
        },
        nullptr, dxfcpp::bit_cast<dxfg_instrument_profile_t *>(instrumentProfile)));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(const std::string &scheduleDefinition) noexcept {
    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&scheduleDefinition) {
            return dxfg_Schedule_getInstance2(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                              scheduleDefinition.c_str());
        },
        nullptr, scheduleDefinition));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile,
                                                   const std::string &venue) noexcept {
    if (!instrumentProfile) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile, auto &&venue) {
            return dxfg_Schedule_getInstance3(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                              instrumentProfile, venue.c_str());
        },
        nullptr, dxfcpp::bit_cast<dxfg_instrument_profile_t *>(instrumentProfile), venue));
}

std::vector<std::string> Schedule::getTradingVenues(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept {
    std::vector<std::string> result{};

    if (!instrumentProfile) {
        return result;
    }

    auto graalStringList = runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile) {
            return dxfg_Schedule_getTradingVenues(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                  instrumentProfile);
        },
        nullptr, dxfcpp::bit_cast<dxfg_instrument_profile_t *>(instrumentProfile));

    if (!graalStringList) {
        return result;
    }

    for (auto i = 0; i < graalStringList->size; i++) {
        result.push_back(dxfcpp::toString(graalStringList->elements[i]));
    }

    StringList::release(graalStringList);

    return result;
}

void Schedule::downloadDefaults(const std::string &downloadConfig) noexcept {
    runIsolatedOrElse(
        [](auto threadHandle, auto &&downloadConfig) {
            return dxfg_Schedule_downloadDefaults(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                  downloadConfig.c_str()) == 0;
        },
        false, downloadConfig);
}

bool Schedule::setDefaults(const std::vector<char> &data) noexcept {
    return runIsolatedOrElse(
        [](auto threadHandle, auto &&data) {
            auto size = data.size() > std::numeric_limits<std::int32_t>::max()
                            ? std::numeric_limits<std::int32_t>::max()
                            : static_cast<std::int32_t>(data.size());

            return dxfg_Schedule_setDefaults(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), data.data(),
                                             size) == 0;
        },
        false, data);
}

/* dxfg_session_t* */ void *Schedule::getSessionByTime(/* dxfg_schedule_t* */ void *schedule,
                                                       std::int64_t time) noexcept {
    if (!schedule) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time) {
            return dxfg_Schedule_getSessionByTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule,
                                                  time);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), time));
}

/* dxfg_day_t* */ void *Schedule::getDayByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time) noexcept {
    if (!schedule) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time) {
            return dxfg_Schedule_getDayByTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule, time);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), time));
}

/* dxfg_day_t* */ void *Schedule::getDayById(/* dxfg_schedule_t* */ void *schedule, std::int32_t dayId) noexcept {
    if (!schedule) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&dayId) {
            return dxfg_Schedule_getDayById(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule, dayId);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), dayId));
}

/* dxfg_day_t* */ void *Schedule::getDayByYearMonthDay(/* dxfg_schedule_t* */ void *schedule,
                                                       std::int32_t yearMonthDay) noexcept {
    if (!schedule) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&yearMonthDay) {
            return dxfg_Schedule_getDayByYearMonthDay(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule,
                                                      yearMonthDay);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), yearMonthDay));
}

/* dxfg_session_t* */ void *Schedule::getNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                              /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!schedule || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time, auto &&filter) {
            return dxfg_Schedule_getNearestSessionByTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                         schedule, time, filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), time,
        dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Schedule::findNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                               /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!schedule || !filter) {
        return nullptr;
    }

    return dxfcpp::bit_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time, auto &&filter) {
            return dxfg_Schedule_findNearestSessionByTime(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                          schedule, time, filter);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule), time,
        dxfcpp::bit_cast<dxfg_session_filter_t *>(filter)));
}

std::string Schedule::getName(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getName(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    String::release(string);

    return result;
}

std::string Schedule::getTimeZoneDisplayName(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getTimeZone(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    String::release(string);

    return result;
}

std::string Schedule::getTimeZoneId(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getTimeZone_getID(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, dxfcpp::bit_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    String::release(string);

    return result;
}

} // namespace schedule
} // namespace dxfcpp::isolated