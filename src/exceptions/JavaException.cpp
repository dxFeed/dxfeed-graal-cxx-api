// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/JavaException.hpp"

#include "../internal/CallbackBarrier.hpp"
#include "../internal/Isolate.hpp"
#include "../isolated/IsolatedCommon.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

namespace {

void releaseException(void *exceptionHandle) noexcept {
    runRelease("JavaException", [&] {
        runIsolatedThrow(
            [](auto threadHandle, auto *exception) {
                dxfg_v4_Exception_release(static_cast<graal_isolatethread_t *>(threadHandle), exception);
            },
            static_cast<dxfg_v4_exception_t *>(exceptionHandle));
    });
}

// Throws the Java exception of the calling thread, if it has one.
void throwIfThreadExceptionExists(graal_isolatethread_t *thread) {
    auto exception = isolated::fetchThreadException(thread);

    if (!exception) {
        throw RuntimeException("Unable to get the Java exception of a failed dxFeed Graal SDK function");
    }

    if (*exception != nullptr) {
        throw JavaException::create(exception->get());
    }
}

} // namespace

namespace isolated {

void ThreadExceptionDeleter::operator()(dxfg_v4_exception_t *exception) const noexcept {
    dxfg_v4_Exception_release(thread, exception);
}

std::optional<ThreadException> fetchThreadException(graal_isolatethread_t *thread) noexcept {
    dxfg_v4_exception_t *exception = nullptr;

    if (dxfg_v4_get_and_clear_thread_exception(thread, &exception) == DXFG_EXECUTE_SUCCESSFULLY) {
        return ThreadException{exception, ThreadExceptionDeleter{thread}};
    }

    // The SDK failed to convert the exception and stored the exception of that failure instead: take it too, so that a
    // later call does not get it. Its status is not checked, there is nothing more to do.
    dxfg_v4_exception_t *conversionException = nullptr;

    if (dxfg_v4_get_and_clear_thread_exception(thread, &conversionException) == DXFG_EXECUTE_SUCCESSFULLY) {
        dxfg_v4_Exception_release(thread, conversionException);
    }

    return std::nullopt;
}

void throwThreadException(graal_isolatethread_t *thread) {
    throwIfThreadExceptionExists(thread);

    throw RuntimeException("Unexpected failure of a dxFeed Graal SDK function without a Java exception");
}

void throwUnexpectedNullResult() {
    throw RuntimeException("Unexpected null result of a dxFeed Graal SDK function without a Java exception");
}

void clearThreadException(graal_isolatethread_t *thread) noexcept {
    // The exception, if any, is released at once.
    static_cast<void>(fetchThreadException(thread));
}

} // namespace isolated

struct JavaException::JavaData {
    std::string className;
    std::string message;
    std::string stackTrace;
};

JavaException::JavaException(StringParam message, StringParam className, StringParam stackTrace)
    : RuntimeException(fmt::format("Java exception of type '{}' was thrown. {}", className.view(), message.view()),
                       stackTrace),
      javaData_(std::make_shared<const JavaData>(
          JavaData{std::string(className.view()), std::string(message.view()),
                   std::string(stackTrace.view())})) {
}

JavaException::JavaException(const JavaException &other) noexcept
    : RuntimeException(other), javaData_(other.javaData_) {
}

const std::string &JavaException::getJavaClassName() const & noexcept {
    return javaData_->className;
}

const std::string &JavaException::getJavaMessage() const & noexcept {
    return javaData_->message;
}

const std::string &JavaException::getJavaStackTrace() const & noexcept {
    return javaData_->stackTrace;
}

JavaException JavaException::create(void *exceptionHandle) {
    if (exceptionHandle == nullptr) {
        return {"null", "", ""};
    }

    auto *exception = dxfcpp::bit_cast<dxfg_v4_exception_t *>(exceptionHandle);

    return {toString(exception->message), toString(exception->class_name), toString(exception->print_stack_trace)};
}

JavaException JavaException::createAndRelease(void *exceptionHandle) {
    DXFCPP_FINALLY([exceptionHandle] {
        releaseException(exceptionHandle);
    });

    return create(exceptionHandle);
}

void JavaException::throwIfJavaThreadExceptionExists() {
    runIsolatedThrow([](auto threadHandle) {
        throwIfThreadExceptionExists(static_cast<graal_isolatethread_t *>(threadHandle));
    });
}

void JavaException::throwException() {
    // The function always fails: it writes no object.
    static_cast<void>(isolated::runGraalV4FunctionWithResult(dxfg_v4_throw_exception));
}

DXFCPP_END_NAMESPACE
