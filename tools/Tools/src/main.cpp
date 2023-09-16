// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include "Connect/ConnectTool.hpp"
#include "Dump/DumpTool.hpp"
#include "Help/HelpTool.hpp"
#include "LatencyTest/LatencyTestTool.hpp"
#include "PerfTest/PerfTestTool.hpp"

#include <variant>

#include <process/process.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using namespace std::literals;
using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace ttldtor::process;

using Tool =
    std::variant<tools::ConnectTool, tools::DumpTool, tools::HelpTool, tools::LatencyTest, tools::PerfTestTool>;

auto getVersion() noexcept {
    return "Tools " DXFCXX_VERSION;
}

void showUsage(auto &&tools) noexcept {
    std::cout << getVersion() << "\n\nUsage:\n  Tools <tool> [...]\n\nWhere <tool> is one of:\n";

    auto maxNameSize = ranges::max(tools | ranges::views::transform([](auto &&tool) {
                                       return std::visit(
                                           [](auto &&t) {
                                               return t.getName().size();
                                           },
                                           tool);
                                   }));

    for (auto &&tool : tools) {
        std::visit(
            [maxNameSize](auto &&t) {
                fmt::print("  {:{}}  {}\n", t.getName(), maxNameSize, t.getShortDescription());
            },
            tool);
    }

    std::cout << "\nTo get detailed help on some tool use \"Help <tool>\".\n";
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args{};

    if (argc > 1) {
        for (int argIndex = 1; argIndex < argc; argIndex++) {
            args.emplace_back(argv[argIndex]);
        }
    }

    std::vector<Tool> tools{tools::ConnectTool{}, tools::DumpTool{}, tools::HelpTool{}, tools::LatencyTest{},
                            tools::PerfTestTool{}};

    std::cout << std::thread::hardware_concurrency() << std::endl;

    tools::DumpTool{}.run(tools::DumpTool::Args{"demo.dxfeed.com:7300", "Quote", "AAPL"});

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        showUsage(tools);

        return 0;
    }

    if (args[0] == "--version") {
        std::cout << getVersion() << "\n";

        return 0;
    }

    //    std::cout << "Memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    //    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
    //
    //    dxfcpp::DXFeed::getInstance();
    //
    //    std::cout << "Memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    //    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
}