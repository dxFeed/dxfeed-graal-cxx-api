// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#ifdef DXFCXX_FEATURE_STACKTRACE
#    include <boost/stacktrace.hpp>
#endif

#include <fmt/format.h>

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
#endif

RuntimeException::RuntimeException(const StringLike &message, const StringLike &additionalStackTrace)
    : std::runtime_error(message.c_str()),
#ifdef DXFCXX_FEATURE_STACKTRACE
      stackTrace_(additionalStackTrace.empty() ? stackTraceToString(boost::stacktrace::stacktrace())
                                               : fmt::format("{}\n{}", additionalStackTrace.c_str(),
                                                             stackTraceToString(boost::stacktrace::stacktrace())))
#else
      stackTrace_(additionalStackTrace)
#endif
{
}

RuntimeException::RuntimeException(const RuntimeException &other) noexcept = default;

const std::string &RuntimeException::getStackTrace() const & {
    return stackTrace_;
}

DXFCPP_END_NAMESPACE

std::ostream &operator<<(std::ostream &os, const dxfcpp::RuntimeException &e) {
    return os << fmt::format("{}\n{}", e.what(), e.getStackTrace());
}
