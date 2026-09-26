// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#define DOCTEST_CONFIG_IMPLEMENT

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <doctest.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#endif

#if defined(__has_feature)
#    if __has_feature(thread_sanitizer)
#        define DXFC_TEST_TSAN 1
#    endif
#    if __has_feature(address_sanitizer)
#        define DXFC_TEST_ASAN 1
#    endif
#endif

#if defined(__SANITIZE_THREAD__)
#    define DXFC_TEST_TSAN 1
#endif

#if defined(__SANITIZE_ADDRESS__)
#    define DXFC_TEST_ASAN 1
#endif

// LeakSanitizer is a part of ASan on Linux.
#if defined(DXFC_TEST_ASAN) && defined(__linux__) && __has_include(<sanitizer/lsan_interface.h>)
#    include <sanitizer/lsan_interface.h>
#    define DXFC_TEST_LSAN 1
#endif

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

// Workaround for P0-4 (static analysis report; fixed in the lifetime ticket, which removes this function): after all
// tests have passed, the process may hang or crash in static destruction and in the Graal isolate teardown. So the
// test process ends without them: std::_Exit, and TerminateProcess on Windows, where the DLL detach is skipped too.
// The leak check of LeakSanitizer normally runs at exit, so it is run explicitly. Not used under ThreadSanitizer:
// it sets the exit code for the reported races only at the normal exit.
[[noreturn]] void exitWithoutTeardown(int code) {
    std::cout.flush();
    std::cerr.flush();
    std::fflush(nullptr);

#ifdef DXFC_TEST_LSAN
    __lsan_do_leak_check(); // terminates the process with an error code if there are leaks
#endif

#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));
#endif

    std::_Exit(code);
}

int finish(int code) {
#ifdef DXFC_TEST_TSAN
    return code;
#else
    exitWithoutTeardown(code);
#endif
}

int main(int argc, char **argv) {
    setSignalHandler();

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    const int res = context.run(); // run queries, or run tests unless --no-run is specified

    if (context.shouldExit())  // important - query flags (and --exit) rely on the user doing this
        return finish(res);    // propagate the result of the tests

    context.clearFilters(); // removes all filters added up to this point

    return finish(res);
}
