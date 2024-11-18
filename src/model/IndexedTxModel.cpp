// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/model/IsolatedIndexedTxModel.hpp>
#include <dxfeed_graal_cpp_api/model/IndexedTxModel.hpp>

DXFCPP_BEGIN_NAMESPACE

EventSourceWrapper EventSourceWrapper::fromGraal(void *graalNative) {
    switch (static_cast<dxfg_indexed_event_source_t *>(graalNative)->type) {
    case ORDER_SOURCE:
        return OrderSource::valueOf(static_cast<dxfg_indexed_event_source_t *>(graalNative)->id);
    default:
        return IndexedEventSource::fromGraal(graalNative);
    }
}

DXFCPP_END_NAMESPACE