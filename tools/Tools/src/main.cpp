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

auto getVersion() noexcept {
    return "Tools " DXFCXX_VERSION;
}

struct Tools {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<tools::HelpTool::Tool> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return "Tools " DXFCXX_VERSION;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return "Tools " DXFCXX_VERSION;
    }
};

const std::string Tools::NAME{"Tools"};
const std::string Tools::SHORT_DESCRIPTION{"A collection of useful utilities that use the dxFeed API."};
const std::string Tools::DESCRIPTION{};
const std::vector<std::string> Tools::USAGE{
    NAME + " <tool> [...]",
};
const std::vector<std::string> Tools::ADDITIONAL_INFO{
    {R"(To get detailed help on some tool use "Help <tool>".)"},
};

const std::vector<tools::HelpTool::Tool> Tools::ARGS{tools::HelpTool::ALL_TOOLS | ranges::views::values |
                                                     ranges::to<std::vector>};

int main(int argc, char *argv[]) {
    const auto usage = tools::HelpTool::generateHelpScreen<Tools>();

    std::vector<std::string> args{};

    if (argc > 1) {
        for (int argIndex = 1; argIndex < argc; argIndex++) {
            args.emplace_back(argv[argIndex]);
        }
    }

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        std::cout << usage << std::endl;

        return 0;
    }

    if (args[0] == "--version") {
        std::cout << getVersion() << "\n";

        return 0;
    }

    for (auto &&[name, tool] : tools::HelpTool::ALL_TOOLS) {
        if (iEquals(args[0], name)) {
            std::visit(
                [&args]<typename Tool>(Tool &&t) {
                    using T = std::decay_t<Tool>;

                    auto result = T::Args::parse(args | ranges::views::drop(1) | ranges::to<std::vector>);

                    if (result.isHelp) {
                        std::cout << tools::HelpTool::generateHelpScreen<T>() << "\n";

                        return;
                    }

                    if (result.isError) {
                        std::cout << tools::HelpTool::generateHelpScreen<T>(result.errorString) << "\n";

                        return;
                    }
                },
                tool);

            return 0;
        }
    }

    std::cout << getVersion() << "\n";

    return 0;
}