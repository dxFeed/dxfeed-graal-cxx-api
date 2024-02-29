// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/format.h>

namespace dxfcpp {

void JavaException::throwIfJavaThreadExceptionExists() {
    dxfg_exception_t *exception = runIsolatedThrow([](auto threadHandle) {
        return dxfg_get_and_clear_thread_exception_t(static_cast<graal_isolatethread_t *>(threadHandle));
    });

    if (exception == nullptr) {
        return;
    }

    auto message = toString(exception->message);
    auto className = toString(exception->class_name);
    auto stackTrace = toString(exception->print_stack_trace);

    runIsolatedThrow(
        [](auto threadHandle, auto &&...params) {
            dxfg_Exception_release(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        exception);

    throw JavaException(message, className, stackTrace);
}

JavaException::JavaException(const std::string &message, const std::string &className, std::string stackTrace)
    : std::runtime_error(fmt::format("Java exception of type '{}' was thrown. {}", className, message)),
      stackTrace(std::move(stackTrace)) {
}

} // namespace dxfcpp