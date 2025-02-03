// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

DXFCPP_BEGIN_NAMESPACE

InPlaceExecutor::InPlaceExecutor(LockExternalConstructionTag) {
}

InPlaceExecutor::~InPlaceExecutor() {
}

std::shared_ptr<InPlaceExecutor> InPlaceExecutor::create() {
    auto executor = InPlaceExecutor::createShared();

    executor->handle_ = isolated::executors::IsolatedInPlaceExecutor::create();

    return executor;
}

void InPlaceExecutor::processAllPendingTasks() const {
    std::lock_guard lock(mutex_);

    isolated::executors::IsolatedInPlaceExecutor::processAllPendingTasks(handle_);
}

const JavaObjectHandle<ExecutorTag> &InPlaceExecutor::getHandle() const {
    std::lock_guard lock(mutex_);

    return handle_;
}

DXFCPP_END_NAMESPACE