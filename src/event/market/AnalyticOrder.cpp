// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

namespace dxfcpp {

const EventTypeEnum &AnalyticOrder::TYPE = EventTypeEnum::ANALYTIC_ORDER;

void AnalyticOrder::fillData(void *graalNative) noexcept {
    Order::fillData(graalNative);

    auto graalAnalyticOrder = bit_cast<dxfg_analytic_order_t *>(graalNative);

    analyticOrderData_ = {graalAnalyticOrder->iceberg_peak_size, graalAnalyticOrder->iceberg_hidden_size,
                          graalAnalyticOrder->iceberg_executed_size, graalAnalyticOrder->iceberg_flags};
}

std::shared_ptr<AnalyticOrder> AnalyticOrder::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    if (bit_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_ANALYTIC_ORDER) {
        return {};
    }

    try {
        auto analyticOrder = std::make_shared<AnalyticOrder>();

        analyticOrder->fillData(graalNative);

        return analyticOrder;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::string AnalyticOrder::toString() const noexcept {
    return fmt::format(
        "AnalyticOrder{{{}, icebergPeakSize={}, icebergHiddenSize={}, icebergExecutedSize={}, icebergType={}}}",
        baseFieldsToString(), dxfcpp::toString(getIcebergPeakSize()), dxfcpp::toString(getIcebergHiddenSize()),
        dxfcpp::toString(getIcebergExecutedSize()), getIcebergType().toString());
}

} // namespace dxfcpp