// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp {

template <typename T> void JavaObjectHandler<T>::deleter(void *handler) noexcept {
    runIsolatedOrElse(
        [handler = handler](auto threadHandle) {
            if (handler) {
                return dxfg_JavaObjectHandler_release(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), bit_cast<dxfg_java_object_handler *>(handler)) == 0;
            }

            return true;
        },
        false);
}

template struct JavaObjectHandler<DXEndpoint>;
template struct JavaObjectHandler<DXEndpointStateChangeListener>;
template struct JavaObjectHandler<DXFeed>;
template struct JavaObjectHandler<DXFeedSubscription>;
template struct JavaObjectHandler<DXFeedEventListener>;
template struct JavaObjectHandler<DXEndpoint::Builder>;

} // namespace dxfcpp