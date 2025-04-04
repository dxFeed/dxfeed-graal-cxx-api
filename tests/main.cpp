// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>
#include <string>

#ifdef WIN32
void setSignalHandler() {
}
#else
#    include <execinfo.h>
#    include <signal.h>

#    ifdef DXFCXX_FEATURE_STACKTRACE
#        include <boost/stacktrace.hpp>
#        include <fmt/format.h>

inline std::string boostStackTraceToString(const boost::stacktrace::stacktrace &stacktrace) {
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

std::string getStackTrace() {
    return boostStackTraceToString(boost::stacktrace::stacktrace());
}
#    else
std::string getStackTrace() {
    return "";
}
#    endif

void sigSegvHandler(int) {
    std::cerr << "SIGSEGV:\n" << getStackTrace() << std::endl;

    exit(1);
}

void sigBusHandler(int) {
    std::cerr << "SIGBUS:\n" << getStackTrace() << std::endl;

    exit(1);
}

void setSignalHandler() {
    signal(SIGSEGV, sigSegvHandler);
    signal(SIGBUS, sigBusHandler);
}
#endif

int main(int /*argc*/, char ** /*argv*/) {
    setSignalHandler();

    doctest::Context context;

    const int res = context.run(); // run queries, or run tests unless --no-run is specified

    if (context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;           // propagate the result of the tests

    context.clearFilters(); // removes all filters added up to this point

    return res;
}