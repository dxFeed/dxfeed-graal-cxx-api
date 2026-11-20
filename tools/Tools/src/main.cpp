// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Connect/ConnectTool.hpp"
#include "Dump/DumpTool.hpp"
#include "Help/HelpTool.hpp"
#include "Tools.hpp"
#include "ToolsProperties.hpp"
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wdeprecated-declarations")
#include <range/v3/all.hpp>
DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_MSC_WARNINGS_POP()

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// A tool with its own process contract: a static function that takes the arguments after the name of the tool and the
// streams of the output and the errors, and returns the exit code.
template <typename T>
concept HasProcessEntry = requires(const std::vector<std::string> &args, std::ostream &out, std::ostream &err) {
    { T::runProcess(args, out, err) } -> std::same_as<int>;
};

using namespace std::literals;
using namespace dxfcpp;
using namespace dxfcpp::literals;

int main(int argc, char *argv[]) {
    try {
        tools::setToolsSystemProperties();

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

            // A tool whose run returns an int sets the exit code (Diagnose); the others exit with 0.
            int exitCode = 0;

            std::visit(
                [&args, &exitCode]<typename Tool>(Tool &&) {
                    using T = std::decay_t<Tool>;

                    // A tool with its own process contract (the command line, the output and the exit codes of a
                    // specification) takes the raw arguments and the streams, and returns the exit code.
                    if constexpr (HasProcessEntry<T>) {
                        try {
                            exitCode = T::runProcess(args | ranges::views::drop(1) | ranges::to<std::vector>,
                                                     std::cout, std::cerr);
                        } catch (const RuntimeException &e) {
                            std::cerr << e << '\n';
                            exitCode = 1;
                        }

                        return;
                    } else {
                        constexpr bool returnsExitCode =
                            std::is_same_v<decltype(T::run(std::declval<const typename T::Args &>())), int>;

                        try {
                            auto parseResult = T::Args::parse(args | ranges::views::drop(1) | ranges::to<std::vector>);

                            if (parseResult.isHelp) {
                                std::cout << tools::Tools::getName() << "\n";
                                std::cout << tools::HelpTool::generateToolHelpScreen<T>() << "\n";

                                return;
                            }

                            if (parseResult.isError) {
                                std::cout << tools::Tools::getName() << "\n";
                                std::cout << tools::HelpTool::generateToolHelpScreen<T>(parseResult.errorString)
                                          << "\n";

                                if constexpr (returnsExitCode) {
                                    exitCode = 1;
                                }

                                return;
                            }

                            // std::cout << tools::Tools::getName() << "\n";
                            if constexpr (returnsExitCode) {
                                exitCode = T::run(parseResult.result);
                            } else {
                                T::run(parseResult.result);
                            }
                        } catch (const RuntimeException &e) {
                            std::cerr << e << '\n';

                            if constexpr (returnsExitCode) {
                                exitCode = 1;
                            }
                        }
                    }
                },
                tool);

            return exitCode;
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