// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include "Connect/ConnectTool.hpp"
#include "Dump/DumpTool.hpp"
#include "Help/HelpTool.hpp"
#include "LatencyTest/LatencyTestTool.hpp"
#include "PerfTest/PerfTestTool.hpp"
#include "Tools.hpp"

#include <variant>

#include <process/process.hpp>

#include <range/v3/all.hpp>

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using namespace std::literals;
using namespace dxfcpp;
using namespace dxfcpp::literals;

int main(int argc, char *argv[]) {
    try {
        System::setProperty(DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY, "true");
        // Enable experimental feature.
        System::setProperty("dxfeed.experimental.dxlink.enable", "true");
        // Set scheme for dxLink.
        System::setProperty("scheme", "ext:opt:sysprops,resource:dxlink.xml");

        const auto usage = tools::HelpTool::generateToolHelpScreen<tools::Tools>();

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
            std::cout << tools::Tools::getName() << "\n";

            return 0;
        }

        for (auto &&[name, tool] : tools::HelpTool::ALL_TOOLS) {
            if (!iEquals(args[0], name)) {
                continue;
            }

            std::visit(
                [&args]<typename Tool>(Tool &&) {
                    try {
                        using T = std::decay_t<Tool>;
                        auto parseResult = T::Args::parse(args | ranges::views::drop(1) | ranges::to<std::vector>);

                        if (parseResult.isHelp) {
                            std::cout << tools::Tools::getName() << "\n";
                            std::cout << tools::HelpTool::generateToolHelpScreen<T>() << "\n";

                            return;
                        }

                        if (parseResult.isError) {
                            std::cout << tools::Tools::getName() << "\n";
                            std::cout << tools::HelpTool::generateToolHelpScreen<T>(parseResult.errorString) << "\n";

                            return;
                        }

                        std::cout << tools::Tools::getName() << "\n";
                        T::run(parseResult.result);
                    } catch (const RuntimeException &e) {
                        std::cerr << e << '\n';
                    }
                },
                tool);

            return 0;
        }

        std::cout << usage << "\n";
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << '\n';
    } catch (...) {
        std::cerr << "Error!\n";
    }

    return 0;
}