// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/exceptions/RuntimeException.hpp"

#ifdef DXFCXX_FEATURE_STACKTRACE
#    include <boost/stacktrace.hpp>
#endif

#include <fmt/format.h>

#include <memory>
#include <mutex>

DXFCPP_BEGIN_NAMESPACE

#ifdef DXFCXX_FEATURE_STACKTRACE
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

        if (const auto foundIn = frameString.find(" in "); foundIn != std::string::npos) {
            what = frameString.substr(0, foundIn);
            whereStart = foundIn + 4;
        } else {
            if (const auto foundAt = frameString.find(" at "); foundAt != std::string::npos) {
                what = frameString.substr(0, foundAt);
                whereStart = foundAt + 4;
            } else {
                whereStart = frameString.size();
            }
        }

        if (whereStart == frameString.size()) {
            what = frameString;

            result += what + "\n";

            continue;
        }

        std::string where;

        if (const auto foundLastSep = frameString.find_last_of("\\/");
            foundLastSep != std::string::npos && foundLastSep < frameString.size() - 1) {
            where = frameString.substr(foundLastSep + 1);
        } else {
            where = frameString.substr(whereStart);
        }

        result += fmt::format("{}({})\n", what, where);
    }

    return result;
}
#endif

// The frames are captured when the exception is created; resolving them to names (slow: it may run addr2line for
// each frame) is deferred to the first getStackTrace() call.
struct RuntimeException::StackTrace {
    std::string additional{};
#ifdef DXFCXX_FEATURE_STACKTRACE
    boost::stacktrace::stacktrace frames{0, 0}; // empty: the default constructor would capture here
#endif
    std::once_flag formatted{};
    std::string text{};
};

RuntimeException::RuntimeException(const StringLike &message, const StringLike &additionalStackTrace)
    : std::runtime_error(message.c_str()), stackTrace_(std::make_shared<StackTrace>()) {
    stackTrace_->additional = additionalStackTrace;
#ifdef DXFCXX_FEATURE_STACKTRACE
    stackTrace_->frames = boost::stacktrace::stacktrace();
#endif
}

RuntimeException::RuntimeException(const RuntimeException &other) noexcept
    : std::runtime_error(other.what()), stackTrace_(other.stackTrace_) {
}

const std::string &RuntimeException::getStackTrace() const & {
    std::call_once(stackTrace_->formatted, [st = stackTrace_.get()] {
#ifdef DXFCXX_FEATURE_STACKTRACE
        auto frames = stackTraceToString(st->frames);

        st->text = st->additional.empty() ? std::move(frames) : fmt::format("{}\n{}", st->additional, frames);
#else
        st->text = st->additional;
#endif
    });

    return stackTrace_->text;
}

DXFCPP_END_NAMESPACE

std::ostream &operator<<(std::ostream &os, const dxfcpp::RuntimeException &e) {
    return os << fmt::format("{}\n{}", e.what(), e.getStackTrace());
}
