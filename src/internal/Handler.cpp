// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Handler.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"

#include <cstddef>

DXFCPP_BEGIN_NAMESPACE

namespace {

// The calls of the listeners of SimpleHandler in progress on this thread.
constinit thread_local std::size_t threadListenerCallsInProgress{0};

} // namespace

ListenerCallScope::ListenerCallScope() noexcept {
    threadListenerCallsInProgress++;
}

ListenerCallScope::~ListenerCallScope() noexcept {
    threadListenerCallsInProgress--;
}

bool ListenerCallScope::isActive() noexcept {
    return threadListenerCallsInProgress > 0 || Isolate::isInCallback();
}

DXFCPP_END_NAMESPACE
