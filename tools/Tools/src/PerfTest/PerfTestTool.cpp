// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "PerfTestTool.hpp"

namespace dxfcpp::tools {
const std::string PerfTestTool::NAME{"PerfTest"};
const std::string PerfTestTool::SHORT_DESCRIPTION{"Connects to specified address and calculates performance counters."};
const std::string PerfTestTool::DESCRIPTION{R"(
Connects to the specified address(es) and calculates performance counters (events per second, cpu usage, etc).
)"};
const std::vector<std::string> PerfTestTool::USAGE{
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> PerfTestTool::ADDITIONAL_INFO{};

const std::vector<ArgType> PerfTestTool::ARGS{AddressArgRequired{}, TypesArgRequired{}, SymbolsArgRequired{},
                                              PropertiesArg{},      ForceStreamArg{},   CPUUsageByCoreArg{},
                                              DetachListenerArg{},  HelpArg{}};
} // namespace dxfcpp::tools