// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/executors/IsolatedInPlaceExecutor.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::executors {

namespace IsolatedInPlaceExecutor {

// dxfg_executor_t *dxfg_ExecutorBaseOnConcurrentLinkedQueue_new(graal_isolatethread_t *thread);
JavaObjectHandle<ExecutorTag> create() {
    return JavaObjectHandle<dxfcpp::ExecutorTag>{
        runGraalFunctionAndThrowIfNullptr(dxfg_ExecutorBaseOnConcurrentLinkedQueue_new)};
}

// int32_t dxfg_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks(graal_isolatethread_t *thread,
//                                                                         dxfg_executor_t *executor);
void processAllPendingTasks(const JavaObjectHandle<ExecutorTag> &executor) {
    if (!executor) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks`. The "
            "`executor` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks,
                                       static_cast<dxfg_executor_t *>(executor.get()));
}

} // namespace IsolatedInPlaceExecutor

} // namespace isolated::executors

DXFCPP_END_NAMESPACE