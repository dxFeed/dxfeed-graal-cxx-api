// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <boost/stacktrace.hpp>
#include <fmt/format.h>

DXFCPP_BEGIN_NAMESPACE

void JavaException::throwIfJavaThreadExceptionExists() {
    dxfg_exception_t *exception = runIsolatedThrow([](auto threadHandle) {
        return dxfg_get_and_clear_thread_exception_t(static_cast<graal_isolatethread_t *>(threadHandle));
    });

    if (exception == nullptr) {
        return;
    }

    auto javaException = create(exception);

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

std::string stackTraceToString(const boost::stacktrace::stacktrace &stacktrace) {
    std::string result;

    for (auto &&frame : stacktrace) {
        result += "\tat ";
        auto frameString = boost::stacktrace::to_string(frame);

        if (frameString.empty()) {
            result += "\n";

            continue;
        }

        std::string what;
        std::size_t whereStart = 0;

        auto foundIn = frameString.find(" in ");

        if (foundIn != std::string::npos) {
            what = frameString.substr(0, foundIn);
            whereStart = foundIn + 4;
        } else {
            auto foundAt = frameString.find(" at ");

            if (foundAt != std::string::npos) {
                what = frameString.substr(0, foundAt);
                whereStart = foundIn + 4;
            } else {
                whereStart = frameString.size();
            }
        }

        if (whereStart == frameString.size()) {
            what = frameString;

            result += what + "\n";

            continue;
        }

        auto foundLastSep = frameString.find_last_of("\\/");
        std::string where;

        if (foundLastSep != std::string::npos && foundLastSep < frameString.size() - 1) {
            where = frameString.substr(foundLastSep + 1);
        } else {
            where = frameString.substr(whereStart);
        }

        result += fmt::format("{}({})\n", what, where);
    }

    return result;
}

JavaException::JavaException(const std::string &message, const std::string &className, std::string stackTrace)
    : std::runtime_error(fmt::format("Java exception of type '{}' was thrown. {}", className, message)),
      stackTrace_{std::move(stackTrace) + "\n" + stackTraceToString(boost::stacktrace::stacktrace())} {
}

JavaException JavaException::create(void *exceptionHandle) {
    if (exceptionHandle == nullptr) {
        return {"", "", ""};
    }

    auto *exception = dxfcpp::bit_cast<dxfg_exception_t *>(exceptionHandle);

    return {toString(exception->message), toString(exception->class_name), toString(exception->print_stack_trace)};
}

const std::string &JavaException::getStackTrace() const & {
    return stackTrace_;
}

DXFCPP_END_NAMESPACE