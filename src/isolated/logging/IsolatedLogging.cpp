// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/logging/IsolatedLogging.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::logging::IsolatedLogging {

// int32_t dxfg_logging_listener_new(graal_isolatethread_t *thread, dxfg_logging_listener_function_t user_func,
//                                   void *user_data, DXFG_OUT dxfg_logging_listener_t **listener);
JavaObjectHandle<Logging::ListenerTag> createListener(void *userFunc, void *userData) {
    if (!userFunc) {
        throw InvalidArgumentException("Unable to execute function `dxfg_logging_listener_new`. The "
                                       "`userFunc` is nullptr");
    }

    dxfg_logging_listener_t *isolatedListener{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_logging_listener_new,
                                       dxfcpp::bit_cast<dxfg_logging_listener_function_t>(userFunc), userData,
                                       &isolatedListener);

    return JavaObjectHandle<Logging::ListenerTag>(isolatedListener);
}

// int32_t dxfg_logging_set_listener(graal_isolatethread_t *thread, dxfg_logging_listener_t *listener);
void setListener(const JavaObjectHandle<Logging::ListenerTag> &listener) {
    if (!listener) {
        throw InvalidArgumentException("Unable to execute function `dxfg_logging_set_listener`. The "
                                       "`listener` handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_logging_set_listener,
                                       dxfcpp::bit_cast<dxfg_logging_listener_t *>(listener.get()));
}

// int32_t dxfg_logging_set_log_level(graal_isolatethread_t *thread, dxfg_logging_level_t level);
void setLogLevel(Logging::Level level) {
    runGraalFunctionAndThrowIfMinusOne(dxfg_logging_set_log_level, static_cast<dxfg_logging_level_t>(level));
}

// int32_t dxfg_logging_set_err_level(graal_isolatethread_t *thread, dxfg_logging_level_t level);
void setErrFileLevel(Logging::Level level) {
    runGraalFunctionAndThrowIfMinusOne(dxfg_logging_set_err_level, static_cast<dxfg_logging_level_t>(level));
}

} // namespace isolated::logging::IsolatedLogging

DXFCPP_END_NAMESPACE