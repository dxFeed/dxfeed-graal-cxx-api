// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/isolated/metrics/IsolatedMetrics.hpp"

#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp"

#include <dxfg_api.h>

#if __has_include(<dxfg_metrics.h>)
#    include <dxfg_metrics.h>
#endif

DXFCPP_BEGIN_NAMESPACE

namespace isolated::metrics::IsolatedMetrics {

#if __has_include(<dxfg_metrics.h>)
std::string dump() {
    char *text{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_Metrics_dump, &text);

    const auto uText = internal::IsolatedString::toUnique(text);
    auto result = dxfcpp::toString(uText.get());

    return result;
}
#else
std::string dump() {
    return "";
}
#endif

} // namespace isolated::metrics::IsolatedMetrics

DXFCPP_END_NAMESPACE