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
                                           []<typename Tool>(Tool &&t) {
                                               return std::decay_t<Tool>::NAME.size();
                                           },
                                           tool);
                                   }));

    for (auto &&tool : tools) {
        std::visit(
            [maxNameSize]<typename Tool>(Tool &&t) {
                fmt::print("  {:{}}  {}\n", std::decay_t<Tool>::NAME, maxNameSize,
                           std::decay_t<Tool>::SHORT_DESCRIPTION);
            },
            tool);
    }

    std::cout << "\nTo get detailed help on some tool use \"Help <tool>\".\n";
}

int main(int argc, char *argv[]) {
    auto [width, height] = Console::getSize();

    std::cout << width << " x " << height << std::endl;

    using Arg = std::variant<tools::AddressArg, tools::AddressArgRequired, tools::TypesArg, tools::TypesArgRequired,
                             tools::SymbolsArg, tools::SymbolsArgRequired>;

    std::vector<Arg> argz{tools::AddressArg{}, tools::AddressArgRequired{}, tools::TypesArg{},
                          tools::TypesArgRequired{}, tools::SymbolsArg{}, tools::SymbolsArgRequired{}};

    auto maxNameSize = ranges::max(argz | ranges::views::transform([](auto &&arg) {
                                       return std::visit(
                                           []<typename Arg>(Arg &&) {
                                               return std::decay_t<Arg>::getFullName().size();
                                           },
                                           arg);
                                   }));

    for (auto &&arg : argz) {
        std::visit(
            [maxNameSize, width]<typename Arg>(Arg &&) {
                fmt::print("{}", std::decay_t<Arg>::prepareHelp(2, 2 + maxNameSize + 2, width));
            },
            arg);
    }

    return 0;

    std::vector<std::string> args{};

    if (argc > 1) {
        for (int argIndex = 1; argIndex < argc; argIndex++) {
            args.emplace_back(argv[argIndex]);
        }
    }

    std::vector<Tool> tools{tools::ConnectTool{}, tools::DumpTool{}, tools::HelpTool{}, tools::LatencyTest{},
                            tools::PerfTestTool{}};

    // tools::PerfTestTool{}.run(tools::PerfTestTool::Args{"demo.dxfeed.com:7300", "all", "AAPL"});
    // tools::PerfTestTool{}.run(tools::PerfTestTool::Args{"127.0.0.1:6666", "TimeAndSale", "ETH/USD:GDAX"});
    // tools::PerfTestTool{}.run(tools::PerfTestTool::Args{"172.25.32.1:6666", "TimeAndSale", "ETH/USD:GDAX"});
    tools::LatencyTest{}.run(tools::LatencyTest::Args{"demo.dxfeed.com:7300", "all", "AAPL"});

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