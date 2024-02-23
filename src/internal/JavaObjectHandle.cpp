// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

void DefaultDeleter::deleter(void *handle) noexcept {
    if (!handle) {
        return;
    }

    runIsolatedOrElse(
        [](auto threadHandle, auto &&...params) {
            return dxfg_JavaObjectHandler_release(static_cast<graal_isolatethread_t *>(threadHandle), params...) == 0;
        },
        false, static_cast<dxfg_java_object_handler *>(handle));
}

// template <typename T> void JavaObjectHandle<T>::deleter(void *handle) noexcept {
//     if constexpr (Debugger::isDebug) {
//         Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
//     }
//
//     if (!handle) {
//         return;
//     }
//
//     auto result = runIsolatedOrElse(
//         [](auto threadHandle, auto &&...params) {
//             return dxfg_JavaObjectHandler_release(static_cast<graal_isolatethread_t *>(threadHandle), params...) ==
//             0;
//         },
//         false, static_cast<dxfg_java_object_handler *>(handle));
//     dxfcpp::ignore_unused(result);
//
//     if constexpr (Debugger::isDebug) {
//         Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " +
//                         dxfcpp::toString(result));
//     }
// }

template struct JavaObjectHandle<DXEndpoint>;
template struct JavaObjectHandle<DXEndpoint::Builder>;
template struct JavaObjectHandle<DXEndpointStateChangeListener>;

template struct JavaObjectHandle<DXFeed>;
template struct JavaObjectHandle<DXPublisher>;
template struct JavaObjectHandle<DXFeedSubscription>;
template struct JavaObjectHandle<DXFeedEventListener>;

template struct JavaObjectHandle<InstrumentProfileReader>;
template struct JavaObjectHandle<InstrumentProfileCollector>;
template struct JavaObjectHandle<InstrumentProfileConnection>;
template struct JavaObjectHandle<IpfPropertyChangeListener>;
template struct JavaObjectHandle<InstrumentProfileUpdateListener>;
template struct JavaObjectHandle<IterableInstrumentProfile>;

template struct JavaObjectHandle<OnDemandService>;

template struct JavaObjectHandle<Schedule>;
template struct JavaObjectHandle<Day>;
template struct JavaObjectHandle<Session>;
template struct JavaObjectHandle<SessionFilter>;
template struct JavaObjectHandle<DayFilter>;

template struct JavaObjectHandle<TimeFormat>;

} // namespace dxfcpp