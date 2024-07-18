// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>

DXFCPP_BEGIN_NAMESPACE

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

    auto result = static_cast<CEntryPointErrorsEnum>(graal_detach_thread(static_cast<graal_isolatethread_t *>(handle)));

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
        graal_detach_all_threads_and_tear_down_isolate(static_cast<graal_isolatethread_t *>(handle)));

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

Isolate::Isolate() noexcept : mtx_{} {
    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::Isolate()");
    }

    graal_isolate_t *graalIsolateHandle{};
    graal_isolatethread_t *graalIsolateThreadHandle{};

    if (static_cast<CEntryPointErrorsEnum>(graal_create_isolate(
            nullptr, &graalIsolateHandle, &graalIsolateThreadHandle)) == CEntryPointErrorsEnum::NO_ERROR) {

        handle_ = graalIsolateHandle;
        mainIsolateThread_ = std::move(IsolateThread{graalIsolateThreadHandle, true});

        currentIsolateThread_.handle = graalIsolateThreadHandle;
        currentIsolateThread_.isMain = true;
    }

    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::Isolate() -> " + std::string("Isolate{") + dxfcpp::toString(handle_) + ", main = " +
                        mainIsolateThread_.toString() + ", current = " + currentIsolateThread_.toString() + "}");
    }
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
                                "] " + CEntryPointErrorsEnumToStr(result));
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

    return static_cast<void *>(graal_get_current_thread(dxfcpp::bit_cast<graal_isolate_t *>(handle_)));
}

// see isolated/isolated.hpp

namespace isolated {

constexpr auto equalsToZero = [](auto result) {
    return result == 0;
};

constexpr auto runGraalFunction(auto resultCheckerConverter, auto graalFunction, auto defaultValue, auto &&...params) {
    return runIsolatedOrElse(
        [](auto threadHandle, auto &&resultCheckerConverter, auto &&graalFunction, auto &&...params) {
            return resultCheckerConverter(graalFunction(static_cast<graal_isolatethread_t *>(threadHandle), params...));
        },
        defaultValue, resultCheckerConverter, graalFunction, params...);
}

using NativeStringList = typename isolated::internal::NativeStringListWrapper<dxfg_string_list>;

namespace ipf {

/* dxfg_ipf_collector_t* */ void *InstrumentProfileCollector::create() noexcept {
    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle) {
            return dxfg_InstrumentProfileCollector_new(static_cast<graal_isolatethread_t *>(threadHandle));
        },
        nullptr));
}

std::int64_t InstrumentProfileCollector::getLastUpdateTime(
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept {
    if (!instrumentProfileCollectorHandle) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileCollector_getLastUpdateTime(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                     params...);
        },
        0, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle));
}

bool InstrumentProfileCollector::updateInstrumentProfile(
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
    /* dxfg_instrument_profile_t * */ void *ip) noexcept {
    if (!instrumentProfileCollectorHandle || !ip) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileCollector_updateInstrumentProfile(
                       static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle),
        static_cast<dxfg_instrument_profile_t *>(ip));
}

/* dxfg_iterable_ip_t* */ void *
InstrumentProfileCollector::view(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept {
    if (!instrumentProfileCollectorHandle) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileCollector_view(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        nullptr, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle)));
}

bool InstrumentProfileCollector::addUpdateListener(/* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
                                                   /* dxfg_ipf_update_listener_t* */ void *listener) noexcept {
    if (!instrumentProfileCollectorHandle || !listener) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileCollector_addUpdateListener(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                     params...) == 0;
        },
        false, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle),
        static_cast<dxfg_ipf_update_listener_t *>(listener));
}

bool InstrumentProfileCollector::removeUpdateListener(
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle,
    /* dxfg_ipf_update_listener_t* */ void *listener) noexcept {
    if (!instrumentProfileCollectorHandle || !listener) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileCollector_removeUpdateListener(
                       static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle),
        static_cast<dxfg_ipf_update_listener_t *>(listener));
}

/* dxfg_ipf_connection_t* */ void *InstrumentProfileConnection::createConnection(
    const std::string &address,
    /* dxfg_ipf_collector_t* */ void *instrumentProfileCollectorHandle) noexcept {
    if (!instrumentProfileCollectorHandle) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_createConnection(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                     params...);
        },
        nullptr, address.c_str(), static_cast<dxfg_ipf_collector_t *>(instrumentProfileCollectorHandle)));
}

std::string InstrumentProfileConnection::getAddress(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto address = runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_getAddress(static_cast<graal_isolatethread_t *>(threadHandle),
                                                               params...);
        },
        nullptr, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));

    auto result = dxfcpp::toString(address);

    isolated::internal::IsolatedString::release(address);

    return result;
}

std::int64_t InstrumentProfileConnection::getUpdatePeriod(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_getUpdatePeriod(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                    params...);
        },
        0, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::setUpdatePeriod(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
                                                  std::int64_t updatePeriod) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_setUpdatePeriod(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                    params...) == 0;
        },
        false, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle), updatePeriod);
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

    // TODO: Improve error handling
    return dxfcpp::InstrumentProfileConnection::State::NOT_CONNECTED;
}

dxfcpp::InstrumentProfileConnection::State
InstrumentProfileConnection::getState(/* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return dxfcpp::InstrumentProfileConnection::State::CLOSED;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return graalStateToState(dxfg_InstrumentProfileConnection_getState(
                static_cast<graal_isolatethread_t *>(threadHandle), params...));
        },
        dxfcpp::InstrumentProfileConnection::State::CLOSED,
        static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

std::int64_t InstrumentProfileConnection::getLastModified(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_getLastModified(static_cast<graal_isolatethread_t *>(threadHandle),
                                                                    params...);
        },
        0, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::start(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_start(static_cast<graal_isolatethread_t *>(threadHandle),
                                                          params...) == 0;
        },
        false, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::close(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_InstrumentProfileConnection_close(static_cast<graal_isolatethread_t *>(threadHandle),
                                                          params...) == 0;
        },
        false, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle));
}

bool InstrumentProfileConnection::addStateChangeListener(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle,
    /* dxfg_ipf_connection_state_change_listener_t * */ void *listener) noexcept {
    if (!instrumentProfileConnectionHandle || !listener) {
        // TODO: Improve error handling
        return false;
    }

    return runGraalFunction(equalsToZero, dxfg_InstrumentProfileConnection_addStateChangeListener, false,
                            static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle),
                            static_cast<dxfg_ipf_connection_state_change_listener_t *>(listener));
}

bool InstrumentProfileConnection::waitUntilCompleted(
    /* dxfg_ipf_connection_t * */ void *instrumentProfileConnectionHandle, std::int64_t timeoutInMs) noexcept {
    if (!instrumentProfileConnectionHandle) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfileConnectionHandle, auto &&listener) {
            return dxfg_InstrumentProfileConnection_waitUntilCompleted(
                       static_cast<graal_isolatethread_t *>(threadHandle), instrumentProfileConnectionHandle,
                       listener) == 1;
        },
        false, static_cast<dxfg_ipf_connection_t *>(instrumentProfileConnectionHandle), timeoutInMs);
}

/* dxfg_ipf_connection_state_change_listener_t* */ void *
IpfPropertyChangeListener::create(/* dxfg_ipf_connection_state_change_listener_func */ void *userFunc,
                                  void *userData) noexcept {
    if (!userFunc) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&userFunc, auto &&userData) {
            return dxfg_IpfPropertyChangeListener_new(static_cast<graal_isolatethread_t *>(threadHandle), userFunc,
                                                      userData);
        },
        nullptr, dxfcpp::bit_cast<dxfg_ipf_connection_state_change_listener_func>(userFunc), userData));
}

bool InstrumentProfileIterator::hasNext(/* dxfg_iterable_ip_t * */ void *iterable) noexcept {
    if (!iterable) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&iterable) {
            return dxfg_Iterable_InstrumentProfile_hasNext(static_cast<graal_isolatethread_t *>(threadHandle),
                                                           iterable) == 1;
        },
        false, static_cast<dxfg_iterable_ip_t *>(iterable));
}

/* dxfg_instrument_profile_t* */ void *
InstrumentProfileIterator::next(/* dxfg_iterable_ip_t * */ void *iterable) noexcept {
    if (!iterable) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&iterable) {
            return dxfg_Iterable_InstrumentProfile_next(static_cast<graal_isolatethread_t *>(threadHandle), iterable);
        },
        nullptr, static_cast<dxfg_iterable_ip_t *>(iterable)));
}

/* dxfg_ipf_update_listener_t* */ void *
InstrumentProfileUpdateListener::create(/* dxfg_ipf_update_listener_function */ void *userFunc,
                                        void *userData) noexcept {
    if (!userFunc) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&userFunc, auto &&userData) {
            return dxfg_InstrumentProfileUpdateListener_new(static_cast<graal_isolatethread_t *>(threadHandle),
                                                            userFunc, userData);
        },
        nullptr, dxfcpp::bit_cast<dxfg_ipf_update_listener_function>(userFunc), userData));
}

} // namespace ipf

namespace ondemand {

/* dxfg_on_demand_service_t* */ void *OnDemandService::getInstance(/* dxfg_endpoint_t * */ void *endpoint) noexcept {
    if (!endpoint) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&endpoint) {
            return dxfg_OnDemandService_getInstance2(static_cast<graal_isolatethread_t *>(threadHandle), endpoint);
        },
        nullptr, static_cast<dxfg_endpoint_t *>(endpoint)));
}

bool OnDemandService::isReplaySupported(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_isReplaySupported(static_cast<graal_isolatethread_t *>(threadHandle),
                                                          service) == 1;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::isReplay(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_isReplay(static_cast<graal_isolatethread_t *>(threadHandle), service) == 1;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::isClear(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_isClear(static_cast<graal_isolatethread_t *>(threadHandle), service) == 1;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

std::int64_t OnDemandService::getTime(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_getTime(static_cast<graal_isolatethread_t *>(threadHandle), service);
        },
        0, static_cast<dxfg_on_demand_service_t *>(service));
}

double OnDemandService::getSpeed(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return 0.0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_getSpeed(static_cast<graal_isolatethread_t *>(threadHandle), service);
        },
        0.0, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::replay(/* dxfg_on_demand_service_t * */ void *service, std::int64_t time) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service, auto &&time) {
            return dxfg_OnDemandService_replay(static_cast<graal_isolatethread_t *>(threadHandle), service, time) == 0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service), time);
}

bool OnDemandService::replay(/* dxfg_on_demand_service_t * */ void *service, std::int64_t time, double speed) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service, auto &&time, auto &&speed) {
            return dxfg_OnDemandService_replay2(static_cast<graal_isolatethread_t *>(threadHandle), service, time,
                                                speed) == 0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service), time, speed);
}

bool OnDemandService::pause(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_pause(static_cast<graal_isolatethread_t *>(threadHandle), service) == 0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::stopAndResume(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_stopAndResume(static_cast<graal_isolatethread_t *>(threadHandle), service) == 0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::stopAndClear(/* dxfg_on_demand_service_t * */ void *service) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service) {
            return dxfg_OnDemandService_stopAndClear(static_cast<graal_isolatethread_t *>(threadHandle), service) == 0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service));
}

bool OnDemandService::setSpeed(/* dxfg_on_demand_service_t * */ void *service, double speed) noexcept {
    if (!service) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&service, auto &&speed) {
            return dxfg_OnDemandService_setSpeed(static_cast<graal_isolatethread_t *>(threadHandle), service, speed) ==
                   0;
        },
        false, static_cast<dxfg_on_demand_service_t *>(service), speed);
}

} // namespace ondemand

namespace schedule {

/* dxfg_schedule_t* */ void *Day::getSchedule(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getSchedule(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        nullptr, static_cast<dxfg_day_t *>(day)));
}

std::int32_t Day::getDayId(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayId(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getYearMonthDay(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getYearMonthDay(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getYear(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getYear(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getMonthOfYear(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getMonthOfYear(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getDayOfMonth(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayOfMonth(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int32_t Day::getDayOfWeek(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getDayOfWeek(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

bool Day::isHoliday(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isHoliday(static_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

bool Day::isShortDay(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isShortDay(static_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

bool Day::isTrading(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_isTrading(static_cast<graal_isolatethread_t *>(threadHandle), day) == 1;
        },
        false, static_cast<dxfg_day_t *>(day));
}

std::int64_t Day::getStartTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getStartTime(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

std::int64_t Day::getEndTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getEndTime(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

bool Day::containsTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&time) {
            return dxfg_Day_containsTime(static_cast<graal_isolatethread_t *>(threadHandle), day, time) == 1;
        },
        false, static_cast<dxfg_day_t *>(day), time);
}

std::int64_t Day::getResetTime(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getResetTime(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

/* dxfg_session_list* */ void *Day::getSessions(/* dxfg_day_t* */ void *day) {
    if (!day) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_getSessions(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        nullptr, static_cast<dxfg_day_t *>(day)));
}

/* dxfg_session_t* */ void *Day::getSessionByTime(/* dxfg_day_t* */ void *day, std::int64_t time) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&time) {
            return dxfg_Day_getSessionByTime(static_cast<graal_isolatethread_t *>(threadHandle), day, time);
        },
        nullptr, static_cast<dxfg_day_t *>(day), time));
}

/* dxfg_session_t* */ void *Day::getFirstSession(/* dxfg_day_t* */ void *day,
                                                 /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getFirstSession(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::getLastSession(/* dxfg_day_t* */ void *day,
                                                /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getLastSession(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::findFirstSession(/* dxfg_day_t* */ void *day,
                                                  /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findFirstSession(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Day::findLastSession(/* dxfg_day_t* */ void *day,
                                                 /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findLastSession(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::getPrevDay(/* dxfg_day_t* */ void *day,
                                        /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getPrevDay(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::getNextDay(/* dxfg_day_t* */ void *day,
                                        /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_getNextDay(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::findPrevDay(/* dxfg_day_t* */ void *day,
                                         /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findPrevDay(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

/* dxfg_day_t* */ void *Day::findNextDay(/* dxfg_day_t* */ void *day, /* dxfg_day_filter_t* */ void *filter) noexcept {
    if (!day || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&filter) {
            return dxfg_Day_findNextDay(static_cast<graal_isolatethread_t *>(threadHandle), day, filter);
        },
        nullptr, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_filter_t *>(filter)));
}

std::size_t Day::getHashCode(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return dxfcpp::bit_cast<std::size_t>(day);
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_hashCode(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        0, static_cast<dxfg_day_t *>(day));
}

bool Day::equals(/* dxfg_day_t* */ void *day, /* dxfg_day_t* */ void *otherDay) noexcept {
    if (!day || !otherDay) {
        // TODO: Improve error handling
        return false;
    }

    if (day == otherDay) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&day, auto &&otherDay) {
            return dxfg_Day_equals(static_cast<graal_isolatethread_t *>(threadHandle), day, otherDay) == 1;
        },
        false, static_cast<dxfg_day_t *>(day), static_cast<dxfg_day_t *>(otherDay));
}

std::string Day::toString(/* dxfg_day_t* */ void *day) noexcept {
    if (!day) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&day) {
            return dxfg_Day_toString(static_cast<graal_isolatethread_t *>(threadHandle), day);
        },
        nullptr, static_cast<dxfg_day_t *>(day));

    auto result = dxfcpp::toString(string);

    isolated::internal::IsolatedString::release(string);

    return result;
}

/* dxfg_session_filter_t* */ void *SessionFilter::getInstance(std::uint32_t code) noexcept {
    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&filter) {
            return dxfg_SessionFilter_getInstance(static_cast<graal_isolatethread_t *>(threadHandle), filter);
        },
        nullptr, static_cast<dxfg_session_filter_prepare_t>(code)));
}

/* dxfg_day_t* */ void *Session::getDay(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getDay(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        nullptr, static_cast<dxfg_session_t *>(session)));
}

std::int32_t Session::getType(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getType(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, static_cast<dxfg_session_t *>(session));
}

bool Session::isTrading(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_isTrading(static_cast<graal_isolatethread_t *>(threadHandle), session) == 1;
        },
        false, static_cast<dxfg_session_t *>(session));
}

bool Session::isEmpty(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_isEmpty(static_cast<graal_isolatethread_t *>(threadHandle), session) == 1;
        },
        false, static_cast<dxfg_session_t *>(session));
}

std::int64_t Session::getStartTime(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getStartTime(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, static_cast<dxfg_session_t *>(session));
}

std::int64_t Session::getEndTime(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return 0;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_getEndTime(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, static_cast<dxfg_session_t *>(session));
}

bool Session::containsTime(/* dxfg_session_t* */ void *session, std::int64_t time) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return false;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&time) {
            return dxfg_Session_containsTime(static_cast<graal_isolatethread_t *>(threadHandle), session, time) == 1;
        },
        false, static_cast<dxfg_session_t *>(session), time);
}

/* dxfg_session_t* */ void *Session::getPrevSession(/* dxfg_session_t* */ void *session,
                                                    /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_getPrevSession(static_cast<graal_isolatethread_t *>(threadHandle), session, filter);
        },
        nullptr, static_cast<dxfg_session_t *>(session), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::getNextSession(/* dxfg_session_t* */ void *session,
                                                    /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_getNextSession(static_cast<graal_isolatethread_t *>(threadHandle), session, filter);
        },
        nullptr, static_cast<dxfg_session_t *>(session), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::findPrevSession(/* dxfg_session_t* */ void *session,
                                                     /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_findPrevSession(static_cast<graal_isolatethread_t *>(threadHandle), session, filter);
        },
        nullptr, static_cast<dxfg_session_t *>(session), static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Session::findNextSession(/* dxfg_session_t* */ void *session,
                                                     /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!session || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&filter) {
            return dxfg_Session_findNextSession(static_cast<graal_isolatethread_t *>(threadHandle), session, filter);
        },
        nullptr, static_cast<dxfg_session_t *>(session), static_cast<dxfg_session_filter_t *>(filter)));
}

std::size_t Session::getHashCode(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return dxfcpp::bit_cast<std::size_t>(session);
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_hashCode(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        0, static_cast<dxfg_session_t *>(session));
}

bool Session::equals(/* dxfg_session_t* */ void *session, /* dxfg_session_t* */ void *otherSession) noexcept {
    if (!session || !otherSession) {
        // TODO: Improve error handling
        return false;
    }

    if (session == otherSession) {
        return true;
    }

    return runIsolatedOrElse(
        [](auto threadHandle, auto &&session, auto &&otherSession) {
            return dxfg_Session_equals(static_cast<graal_isolatethread_t *>(threadHandle), session, otherSession) == 1;
        },
        false, static_cast<dxfg_session_t *>(session), static_cast<dxfg_session_t *>(otherSession));
}

std::string Session::toString(/* dxfg_session_t* */ void *session) noexcept {
    if (!session) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&session) {
            return dxfg_Session_toString(static_cast<graal_isolatethread_t *>(threadHandle), session);
        },
        nullptr, static_cast<dxfg_session_t *>(session));

    auto result = dxfcpp::toString(string);

    isolated::internal::IsolatedString::release(string);

    return result;
}

/* dxfg_day_filter_t* */ void *DayFilter::getInstance(std::uint32_t code) noexcept {
    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&filter) {
            return dxfg_DayFilter_getInstance(static_cast<graal_isolatethread_t *>(threadHandle), filter);
        },
        nullptr, static_cast<dxfg_day_filter_prepare_t>(code)));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept {
    if (!instrumentProfile) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile) {
            return dxfg_Schedule_getInstance(static_cast<graal_isolatethread_t *>(threadHandle), instrumentProfile);
        },
        nullptr, static_cast<dxfg_instrument_profile_t *>(instrumentProfile)));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(const std::string &scheduleDefinition) noexcept {
    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&scheduleDefinition) {
            return dxfg_Schedule_getInstance2(static_cast<graal_isolatethread_t *>(threadHandle),
                                              scheduleDefinition.c_str());
        },
        nullptr, scheduleDefinition));
}

/* dxfg_schedule_t* */ void *Schedule::getInstance(/* dxfg_instrument_profile_t* */ void *instrumentProfile,
                                                   const std::string &venue) noexcept {
    if (!instrumentProfile) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile, auto &&venue) {
            return dxfg_Schedule_getInstance3(static_cast<graal_isolatethread_t *>(threadHandle), instrumentProfile,
                                              venue.c_str());
        },
        nullptr, static_cast<dxfg_instrument_profile_t *>(instrumentProfile), venue));
}

std::vector<std::string> Schedule::getTradingVenues(/* dxfg_instrument_profile_t* */ void *instrumentProfile) noexcept {
    std::vector<std::string> result{};

    if (!instrumentProfile) {
        // TODO: Improve error handling
        return result;
    }

    auto graalStringList = runIsolatedOrElse(
        [](auto threadHandle, auto &&instrumentProfile) {
            return dxfg_Schedule_getTradingVenues(static_cast<graal_isolatethread_t *>(threadHandle),
                                                  instrumentProfile);
        },
        nullptr, static_cast<dxfg_instrument_profile_t *>(instrumentProfile));

    if (!graalStringList) {
        // TODO: Improve error handling
        return result;
    }

    for (auto i = 0; i < graalStringList->size; i++) {
        result.push_back(dxfcpp::toString(graalStringList->elements[i]));
    }

    isolated::internal::IsolatedStringList::release(graalStringList);

    return result;
}

void Schedule::downloadDefaults(const std::string &downloadConfig) noexcept {
    runIsolatedOrElse(
        [](auto threadHandle, auto &&downloadConfig) {
            return dxfg_Schedule_downloadDefaults(static_cast<graal_isolatethread_t *>(threadHandle),
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

            return dxfg_Schedule_setDefaults(static_cast<graal_isolatethread_t *>(threadHandle), data.data(), size) ==
                   0;
        },
        false, data);
}

/* dxfg_session_t* */ void *Schedule::getSessionByTime(/* dxfg_schedule_t* */ void *schedule,
                                                       std::int64_t time) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time) {
            return dxfg_Schedule_getSessionByTime(static_cast<graal_isolatethread_t *>(threadHandle), schedule, time);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), time));
}

/* dxfg_day_t* */ void *Schedule::getDayByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time) {
            return dxfg_Schedule_getDayByTime(static_cast<graal_isolatethread_t *>(threadHandle), schedule, time);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), time));
}

/* dxfg_day_t* */ void *Schedule::getDayById(/* dxfg_schedule_t* */ void *schedule, std::int32_t dayId) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&dayId) {
            return dxfg_Schedule_getDayById(static_cast<graal_isolatethread_t *>(threadHandle), schedule, dayId);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), dayId));
}

/* dxfg_day_t* */ void *Schedule::getDayByYearMonthDay(/* dxfg_schedule_t* */ void *schedule,
                                                       std::int32_t yearMonthDay) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&yearMonthDay) {
            return dxfg_Schedule_getDayByYearMonthDay(static_cast<graal_isolatethread_t *>(threadHandle), schedule,
                                                      yearMonthDay);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), yearMonthDay));
}

/* dxfg_session_t* */ void *Schedule::getNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                              /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!schedule || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time, auto &&filter) {
            return dxfg_Schedule_getNearestSessionByTime(static_cast<graal_isolatethread_t *>(threadHandle), schedule,
                                                         time, filter);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), time, static_cast<dxfg_session_filter_t *>(filter)));
}

/* dxfg_session_t* */ void *Schedule::findNearestSessionByTime(/* dxfg_schedule_t* */ void *schedule, std::int64_t time,
                                                               /* dxfg_session_filter_t* */ void *filter) noexcept {
    if (!schedule || !filter) {
        // TODO: Improve error handling
        return nullptr;
    }

    return static_cast<void *>(runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule, auto &&time, auto &&filter) {
            return dxfg_Schedule_findNearestSessionByTime(static_cast<graal_isolatethread_t *>(threadHandle), schedule,
                                                          time, filter);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule), time, static_cast<dxfg_session_filter_t *>(filter)));
}

std::string Schedule::getName(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getName(static_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    isolated::internal::IsolatedString::release(string);

    return result;
}

std::string Schedule::getTimeZoneDisplayName(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getTimeZone(static_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    isolated::internal::IsolatedString::release(string);

    return result;
}

std::string Schedule::getTimeZoneId(/* dxfg_schedule_t* */ void *schedule) noexcept {
    if (!schedule) {
        // TODO: Improve error handling
        return dxfcpp::String::EMPTY;
    }

    auto string = runIsolatedOrElse(
        [](auto threadHandle, auto &&schedule) {
            return dxfg_Schedule_getTimeZone_getID(static_cast<graal_isolatethread_t *>(threadHandle), schedule);
        },
        nullptr, static_cast<dxfg_schedule_t *>(schedule));

    auto result = dxfcpp::toString(string);

    isolated::internal::IsolatedString::release(string);

    return result;
}

} // namespace schedule
} // namespace isolated

DXFCPP_END_NAMESPACE