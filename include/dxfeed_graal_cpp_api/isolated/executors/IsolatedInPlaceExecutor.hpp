// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../executors/InPlaceExecutor.hpp"
#include "../../internal/Conf.hpp"
#include "../../internal/JavaObjectHandle.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

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