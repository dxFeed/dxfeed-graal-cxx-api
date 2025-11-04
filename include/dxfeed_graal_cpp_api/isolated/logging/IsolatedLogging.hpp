// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/JavaObjectHandle.hpp"
#include "../../logging/Logging.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::logging::IsolatedLogging {

// int32_t dxfg_logging_listener_new(graal_isolatethread_t *thread, dxfg_logging_listener_function_t user_func,
//                                   void *user_data, DXFG_OUT dxfg_logging_listener_t **listener);
JavaObjectHandle<Logging::ListenerTag> createListener(void *userFunc, void *userData);

// int32_t dxfg_logging_set_listener(graal_isolatethread_t *thread, dxfg_logging_listener_t *listener);
void setListener(const JavaObjectHandle<Logging::ListenerTag> &listener);

// int32_t dxfg_logging_set_log_level(graal_isolatethread_t *thread, dxfg_logging_level_t level);
void setLogLevel(Logging::Level level);

// int32_t dxfg_logging_set_err_level(graal_isolatethread_t *thread, dxfg_logging_level_t level);
void setErrFileLevel(Logging::Level level);

} // namespace isolated::logging::IsolatedLogging

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()