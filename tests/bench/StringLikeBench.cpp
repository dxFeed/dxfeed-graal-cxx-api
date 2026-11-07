// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>
#include "../../src/internal/Common.hpp"

#include <doctest.h>
#include <nanobench.h>

#include <string>

using namespace dxfcpp;

TEST_CASE("Benchmark StringLike") {
    ankerl::nanobench::Bench bench;

    std::string
    testStr = "this is a moderately long test string for benchmarking";

    bench.warmup(100);
    bench.minEpochIterations(2000000);

    // size()
    bench.run("Hybrid size() from string", [&] {
        StringLike h(testStr);
        ankerl::nanobench::doNotOptimizeAway(h.size());
    });

    bench.run("Hybrid size() from string_view", [&] {
        StringLike h{std::string_view(testStr)};
        ankerl::nanobench::doNotOptimizeAway(h.size());
    });

    // empty()
    bench.run("Hybrid empty()", [&] {
        StringLike h("foo");
        ankerl::nanobench::doNotOptimizeAway(h.empty());
    });

    // operator==
    bench.run("Hybrid operator==", [&] {
        StringLike a(testStr), b(testStr);
        ankerl::nanobench::doNotOptimizeAway(a == b);
    });
}