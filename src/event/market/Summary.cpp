// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

const EventTypeEnum &Summary::Type = EventTypeEnum::SUMMARY;

std::shared_ptr<Summary> Summary::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_SUMMARY) {
        return {};
    }

    try {
        auto graalSummary = bit_cast<dxfg_summary_t *>(graalNative);
        auto summary = std::make_shared<Summary>(dxfcpp::toString(graalSummary->market_event.event_symbol));

        summary->setEventTime(graalSummary->market_event.event_time);
        summary->data_ = {graalSummary->day_id,
                          graalSummary->day_open_price,
                          graalSummary->day_high_price,
                          graalSummary->day_low_price,
                          graalSummary->day_close_price,
                          graalSummary->prev_day_id,
                          graalSummary->prev_day_close_price,
                          graalSummary->prev_day_volume,
                          graalSummary->open_interest,
                          graalSummary->flags};

        return summary;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

} // namespace dxfcpp