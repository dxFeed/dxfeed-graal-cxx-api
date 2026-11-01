// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/executors/InPlaceExecutor.hpp"
#include "../../../include/dxfeed_graal_cpp_api/detail/JavaObjectHandle.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::executors {

namespace IsolatedInPlaceExecutor {

// dxfg_executor_t *dxfg_ExecutorBaseOnConcurrentLinkedQueue_new(graal_isolatethread_t *thread);
JavaObjectHandle<ExecutorTag> create();

// int32_t dxfg_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks(graal_isolatethread_t *thread,
//                                                                         dxfg_executor_t *executor);
void processAllPendingTasks(const JavaObjectHandle<ExecutorTag> &executor);

} // namespace IsolatedInPlaceExecutor

} // namespace isolated::executors

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()