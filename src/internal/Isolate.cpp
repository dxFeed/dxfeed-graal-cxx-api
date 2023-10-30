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

    finally([resolvedURL] {
        String::release(resolvedURL);
    });

    return dxfcpp::toString(resolvedURL);
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

    if (!address) {
        return dxfcpp::String::EMPTY;
    }

    finally([address] {
        String::release(address);
    });

    return dxfcpp::toString(address);
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

    finally([graalStringList]{
        StringList::release(graalStringList);
    });

    if (!graalStringList || graalStringList->size == 0) {
        return result;
    }

    for (auto i = 0; i < graalStringList->size; i++) {
        result.push_back(dxfcpp::toString(graalStringList->elements[i]));
    }

    return result;
};

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

} // namespace schedule
} // namespace dxfcpp::isolated