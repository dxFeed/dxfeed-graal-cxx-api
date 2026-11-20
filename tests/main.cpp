// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include <cstdlib>
#include <iostream>
#include <string>

// Sanitizers install their own SIGSEGV/SIGBUS handlers that print the full report. Replacing them with the handlers
// below would hide the report, so they are not installed in sanitized builds.
#if defined(WIN32) || defined(DXFCXX_SANITIZERS_ENABLED)
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

#ifdef DXFCXX_TEST_TEAR_DOWN_ISOLATE
#    include "../src/internal/Isolate.hpp"

#    include <chrono>
#    include <condition_variable>
#    include <cstdio>
#    include <mutex>
#    include <thread>

namespace {

// How long the tear-down may take: a Java thread that ignores the interruption blocks it forever.
constexpr auto TEAR_DOWN_TIMEOUT = std::chrono::seconds(60);

// The exit code of a tear-down that hangs (as timeout(1)) and of a tear-down that is refused or fails.
constexpr int TEAR_DOWN_HANG_EXIT_CODE = 124;
constexpr int TEAR_DOWN_FAILURE_EXIT_CODE = 125;

const char *toText(dxfcpp::Isolate::TearDownResult::Status status) {
    using Status = dxfcpp::Isolate::TearDownResult::Status;

    switch (status) {
    case Status::TORN_DOWN:
        return "torn down";
    case Status::NOTHING_TO_TEAR_DOWN:
        return "nothing to tear down";
    case Status::REFUSED:
        return "refused (a callback of the library is in progress)";
    case Status::FAILED:
        return "failed";
    }

    return "unknown";
}

/**
 * Tears the Graal isolate down, so that LeakSanitizer does not report its memory and reports the leaks of the library
 * through the frames of the SDK. A watchdog ends the process with TEAR_DOWN_HANG_EXIT_CODE if the tear-down hangs.
 *
 * @return `true` if the isolate is torn down or there is nothing to tear down.
 */
bool tearDownIsolate() {
    std::mutex mutex{};
    std::condition_variable done{};
    bool finished = false;

    std::thread watchdog{[&] {
        std::unique_lock lock{mutex};

        if (!done.wait_for(lock, TEAR_DOWN_TIMEOUT, [&] {
                return finished;
            })) {
            std::fprintf(stderr, "[tests] The tear-down of the Graal isolate did not complete in %lld s\n",
                         static_cast<long long>(TEAR_DOWN_TIMEOUT.count()));
            std::fflush(stderr);
            std::_Exit(TEAR_DOWN_HANG_EXIT_CODE);
        }
    }};

    const auto start = std::chrono::steady_clock::now();
    const auto result = dxfcpp::Isolate::shutdownAndTearDown();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    {
        std::lock_guard lock{mutex};

        finished = true;
    }

    done.notify_all();
    watchdog.join();

    std::fprintf(stderr, "[tests] The tear-down of the Graal isolate: %s, error %u, %lld ms\n", toText(result.status),
                 static_cast<unsigned>(result.error), static_cast<long long>(milliseconds));

    using Status = dxfcpp::Isolate::TearDownResult::Status;

    return result.status == Status::TORN_DOWN || result.status == Status::NOTHING_TO_TEAR_DOWN;
}

} // namespace
#endif

int main(int argc, char **argv) {
    setSignalHandler();

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    const int res = context.run(); // run queries, or run tests unless --no-run is specified

    if (context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;           // propagate the result of the tests

    context.clearFilters(); // removes all filters added up to this point

#ifdef DXFCXX_TEST_TEAR_DOWN_ISOLATE
    if (!tearDownIsolate() && res == 0) {
        return TEAR_DOWN_FAILURE_EXIT_CODE;
    }
#endif

    return res;
}
