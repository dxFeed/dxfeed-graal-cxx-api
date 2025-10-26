// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

JavaException::JavaException(const StringLike &message, const StringLike &className,
                             const StringLike &stackTrace)
    : RuntimeException(fmt::format("Java exception of type '{}' was thrown. {}", className.c_str(), message.c_str()),
                       stackTrace) {
}

JavaException::JavaException(const JavaException &other) noexcept = default;

JavaException JavaException::create(void *exceptionHandle) {
    if (exceptionHandle == nullptr) {
        return {"null", "", ""};
    }

    auto *exception = dxfcpp::bit_cast<dxfg_exception_t *>(exceptionHandle);

    return {toString(exception->message), toString(exception->class_name), toString(exception->print_stack_trace)};
}

void JavaException::throwIfJavaThreadExceptionExists() {
    dxfg_exception_t *exception = runIsolatedThrow([](auto threadHandle) {
        return dxfg_get_and_clear_thread_exception_t(static_cast<graal_isolatethread_t *>(threadHandle));
    });

    if (exception == nullptr) {
        return;
    }

    thread_local auto javaException = create(exception);

    runIsolatedThrow(
        [](auto threadHandle, auto &&...params) {
            dxfg_Exception_release(static_cast<graal_isolatethread_t *>(threadHandle), params...);
        },
        exception);

    throw javaException; // NOLINT(*-throw-by-value-catch-by-reference)
}

void JavaException::throwException() {
    runIsolatedThrow([](auto threadHandle) {
        return throwIfNullptr(dxfg_throw_exception(static_cast<graal_isolatethread_t *>(threadHandle)));
    });
}

DXFCPP_END_NAMESPACE