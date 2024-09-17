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
        currentIsolateThread_.handle = graalIsolateThreadHandle;
    }

    if constexpr (Debugger::traceIsolates) {
        Debugger::trace("Isolate::Isolate() -> " + std::string("Isolate{") + dxfcpp::toString(handle_) +
                        ", current = " + currentIsolateThread_.toString() + "}");
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

namespace schedule {

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