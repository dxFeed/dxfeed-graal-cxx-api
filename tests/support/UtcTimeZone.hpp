// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Include this header in the test source of a test executable that compares toString() with the texts recorded in the
// Java API (tests/api/EventsJavaParity.hpp): it pins the time zone of the process to UTC for the whole run. Other test
// executables keep the zone of the machine, so do not include it from TestSupport.hpp.

#pragma once

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

namespace dxfcpp::test {

// toString() prints the times in the default time zone of the Java isolate without a zone suffix, the Java references
// are recorded with the zone UTC. The zone is read at the first use of TimeFormat in the process (TZ is not honored by
// the native SDK on Windows), so it is set when the test run starts, before any test formats a time. Not from a
// dynamic initializer: the library may be linked statically and not yet initialized there.
struct UtcTimeZoneListener : doctest::IReporter {
    explicit UtcTimeZoneListener(const doctest::ContextOptions &) {
    }

    void test_run_start() override {
        System::setProperty("user.timezone", "UTC");
    }

    void report_query(const doctest::QueryData &) override {
    }

    void test_run_end(const doctest::TestRunStats &) override {
    }

    void test_case_start(const doctest::TestCaseData &) override {
    }

    void test_case_reenter(const doctest::TestCaseData &) override {
    }

    void test_case_end(const doctest::CurrentTestCaseStats &) override {
    }

    void test_case_exception(const doctest::TestCaseException &) override {
    }

    void subcase_start(const doctest::SubcaseSignature &) override {
    }

    void subcase_end() override {
    }

    void log_assert(const doctest::AssertData &) override {
    }

    void log_message(const doctest::MessageData &) override {
    }

    void test_case_skipped(const doctest::TestCaseData &) override {
    }
};

} // namespace dxfcpp::test

DOCTEST_REGISTER_LISTENER("utc-time-zone", 0, dxfcpp::test::UtcTimeZoneListener);
