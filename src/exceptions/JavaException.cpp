// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"

#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

namespace {

void releaseStackTraceMethodNames(graal_isolatethread_t *thread, dxfg_exception_t *exception) noexcept {
    if (exception == nullptr) {
        return;
    }

    if (const auto *stackTrace = exception->stack_trace; stackTrace != nullptr && stackTrace->elements != nullptr) {
        for (std::int32_t i = 0; i < stackTrace->size; ++i) {
            auto *element = stackTrace->elements[i];

            if (element != nullptr && element->method_name != nullptr &&
                dxfg_free(thread, const_cast<char *>(element->method_name)) != -1) {
                // Native SDK 3.2.13 does not release this field. Keep the standard release safe for fixed SDKs.
                element->method_name = nullptr;
            }
        }
    }

    releaseStackTraceMethodNames(thread, exception->cause);
}

void releaseException(void *exceptionHandle) noexcept {
    try {
        runIsolatedThrow(
            [](auto threadHandle, auto *exception) {
                auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);

                releaseStackTraceMethodNames(thread, exception);
                dxfg_Exception_release(thread, exception);
            },
            static_cast<dxfg_exception_t *>(exceptionHandle));
    } catch (...) {
        // Cleanup must not replace the Java exception that is already being handled.
    }
}

} // namespace

JavaException::JavaException(const StringLike &message, const StringLike &className, const StringLike &stackTrace)
    : RuntimeException(fmt::format("Java exception of type '{}' was thrown. {}", className.c_str(), message.c_str()),
                       stackTrace) {
}

JavaException::JavaException(const JavaException &other) noexcept : RuntimeException(other) {
}

JavaException JavaException::create(void *exceptionHandle) {
    if (exceptionHandle == nullptr) {
        return {"null", "", ""};
    }

    auto *exception = dxfcpp::bit_cast<dxfg_exception_t *>(exceptionHandle);

    return {toString(exception->message), toString(exception->class_name), toString(exception->print_stack_trace)};
}

JavaException JavaException::createAndRelease(void *exceptionHandle) {
    DXFCPP_FINALLY([exceptionHandle] {
        releaseException(exceptionHandle);
    });

    return create(exceptionHandle);
}

void JavaException::throwIfJavaThreadExceptionExists() {
    dxfg_exception_t *exception = runIsolatedThrow([](auto threadHandle) {
        return dxfg_get_and_clear_thread_exception_t(static_cast<graal_isolatethread_t *>(threadHandle));
    });

    if (exception == nullptr) {
        return;
    }

    auto javaException = createAndRelease(exception);

    throw javaException; // NOLINT(*-throw-by-value-catch-by-reference)
}

void JavaException::throwException() {
    runIsolatedThrow([](auto threadHandle) {
        return throwIfNullptr(dxfg_throw_exception(static_cast<graal_isolatethread_t *>(threadHandle)));
    });
}

DXFCPP_END_NAMESPACE
