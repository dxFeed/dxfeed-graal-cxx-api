// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

template <typename T> void JavaObjectHandle<T>::deleter(void *handle) noexcept {
    auto result = runIsolatedOrElse(
        [handle = handle](auto threadHandle) {
            if constexpr (Debugger::isDebug) {
                Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ")");
            }

            if (handle) {
                return dxfg_JavaObjectHandler_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                      dxfcpp::bit_cast<dxfg_java_object_handler *>(handle)) == 0;
            }

            return true;
        },
        false);
    dxfcpp::ignore_unused(result);

    if constexpr (Debugger::isDebug) {
        Debugger::debug(getDebugName() + "::deleter(handle = " + dxfcpp::toString(handle) + ") -> " + dxfcpp::toString(result));
    }
}

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