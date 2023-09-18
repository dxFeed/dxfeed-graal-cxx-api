// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "LatencyTestTool.hpp"

namespace dxfcpp::tools {
const std::string LatencyTest::NAME{"LatencyTest"};
const std::string LatencyTest::SHORT_DESCRIPTION{"Connects to the specified address(es) and calculates latency."};
const std::string LatencyTest::DESCRIPTION{R"(
Connects to the specified address(es) and calculates latency.
)"};
const std::vector<std::string> LatencyTest::USAGE{
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> LatencyTest::ADDITIONAL_INFO{};
}