// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedMetrics.hpp"

#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

#if __has_include(<dxfg_metrics.h>)
#    include <dxfg_metrics.h>
#endif

DXFCPP_BEGIN_NAMESPACE

namespace isolated::metrics::IsolatedMetrics {

#if __has_include(<dxfg_metrics.h>)
std::string dump() {
    // dxfg_Metrics_dump has no dxfg_v4_ twin and is not deprecated: it returns -1 on a failure.
    char *text = runIsolatedThrow([](auto threadHandle) {
        auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);
        char *result{};

        if (dxfg_Metrics_dump(thread, &result) == -1) {
            throwThreadException(thread);
        }

        return result;
    });

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