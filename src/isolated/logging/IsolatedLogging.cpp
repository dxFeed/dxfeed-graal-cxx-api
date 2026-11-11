// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedLogging.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::logging::IsolatedLogging {

/// dxfg_v4_logging_listener_new
JavaObjectHandle<Logging::ListenerTag> createListener(void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_logging_listener_new`. The "
                                       "`userFunc` is nullptr");
    }

    return JavaObjectHandle<Logging::ListenerTag>(runGraalV4FunctionWithResult(
        dxfg_v4_logging_listener_new, dxfcpp::bit_cast<dxfg_v4_logging_listener_function_t>(userFunc), userData));
}

/// dxfg_v4_logging_set_listener
void setListener(const JavaObjectHandle<Logging::ListenerTag> &listener) {
    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_logging_set_listener`. The "
                                       "`listener` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_logging_set_listener, dxfcpp::bit_cast<dxfg_v4_logging_listener_t *>(listener.get()));
}

/// dxfg_v4_logging_set_log_level
void setLogLevel(Logging::Level level) {
    runGraalV4Function(dxfg_v4_logging_set_log_level, static_cast<dxfg_v4_logging_level_t>(level));
}

/// dxfg_v4_logging_set_err_level
void setErrFileLevel(Logging::Level level) {
    runGraalV4Function(dxfg_v4_logging_set_err_level, static_cast<dxfg_v4_logging_level_t>(level));
}

} // namespace isolated::logging::IsolatedLogging

DXFCPP_END_NAMESPACE