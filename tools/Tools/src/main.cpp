// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>
#include "PerfTest/PerfTestTool.hpp"
#include <process/process.hpp>

#include <variant>

using namespace std::literals;
using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace ttldtor::process;

int main() {
    std::cout << "Memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;

    dxfcpp::DXFeed::getInstance();

    std::cout << "Memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
}