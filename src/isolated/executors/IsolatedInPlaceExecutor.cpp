// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedInPlaceExecutor.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::executors {

namespace IsolatedInPlaceExecutor {

/// dxfg_v4_ExecutorBaseOnConcurrentLinkedQueue_new
JavaObjectHandle<ExecutorTag> create() {
    return JavaObjectHandle<ExecutorTag>{
        runGraalV4FunctionWithNonNullResult(dxfg_v4_ExecutorBaseOnConcurrentLinkedQueue_new)};
}

/// dxfg_v4_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks
void processAllPendingTasks(const JavaObjectHandle<ExecutorTag> &executor) {
    if (!executor) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks`. The "
            "`executor` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_ExecutorBaseOnConcurrentLinkedQueue_processAllPendingTasks,
                       static_cast<dxfg_v4_executor_t *>(executor.get()));
}

} // namespace IsolatedInPlaceExecutor

} // namespace isolated::executors

DXFCPP_END_NAMESPACE