// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/api/IsolatedDXFeed.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::api::IsolatedDXFeed {

/* dxfg_promise_events_t* */ void *getTimeSeriesPromise(/* dxfg_feed_t * */ const JavaObjectHandle<DXFeed> &feed,
                                                        /* dxfg_event_clazz_t */ const EventTypeEnum &eventType,
                                                        /* dxfg_symbol_t * */ const SymbolWrapper &symbol,
                                                        std::int64_t fromTime, std::int64_t toTime) {
    if (!feed) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_DXFeed_getTimeSeriesPromise`. The `feed` handle is invalid");
    }

    auto graalSymbol = symbol.toGraal();

    auto result = dxfcpp::bit_cast<void *>(
        runGraalFunctionAndThrowIfNullptr(dxfg_DXFeed_getTimeSeriesPromise, static_cast<dxfg_feed_t *>(feed.get()),
                                          static_cast<dxfg_event_clazz_t>(eventType.getId()),
                                          static_cast<dxfg_symbol_t *>(graalSymbol), fromTime, toTime));

    SymbolWrapper::freeGraal(graalSymbol);

    return result;
}

} // namespace isolated::api::IsolatedDXFeed

DXFCPP_END_NAMESPACE