// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

template <typename T> void JavaObjectHandle<T>::deleter(void *handle) noexcept {
    runIsolatedOrElse(
        [handle = handle](auto threadHandle) {
            if (handle) {
                return dxfg_JavaObjectHandler_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle),
                                                      dxfcpp::bit_cast<dxfg_java_object_handler *>(handle)) == 0;
            }

            return true;
        },
        false);
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

} // namespace dxfcpp