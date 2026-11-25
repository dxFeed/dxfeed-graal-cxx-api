// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../../include/dxfeed_graal_cpp_api/api.hpp"
#include "../Args/Args.hpp"
#include "Args.hpp"
#include "Run.hpp"

#include <fmt/format.h>

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace dxfcpp::tools {

/// The load generator and the lag meter of the NetTest specification: the command line, the exit codes and the output
/// follow the specification, so the tool does not use the common parser of the arguments. The process calls
/// prepareProcess() first, before the isolate exists, then runs the tool through runProcess() with the raw arguments
/// (see main.cpp).
struct NetTestTool {
    /// The exit code of a command line error.
    static constexpr int EXIT_BAD_ARGUMENTS = 2;

    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<ArgType> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return NAME;
    }

    [[nodiscard]] static std::string prepareHelp(std::size_t namePadding,
                                                 std::size_t nameFieldSize /* padding + name + padding */,
                                                 std::size_t) noexcept {
        return fmt::format("{:{}}{:<{}}{:{}}{}\n", "", namePadding, getFullName(), nameFieldSize - 2 * namePadding, "",
                           namePadding, SHORT_DESCRIPTION);
    }

    /// Prepares the process before anything creates the Graal isolate: a valid command line with the options of the
    /// isolate (-X<option>, -XX:<option>) creates it with them. Returns the exit code when the process must end: 2 when
    /// the isolate rejects one of the options (after its reason and the line `NetTest: the isolate rejected one of the
    /// options: ...` on `err`), 1 when it cannot be created otherwise. An invalid command line and --help are left to
    /// runProcess().
    static std::optional<int> prepareProcess(const std::vector<std::string> &args, std::ostream &err) {
        const auto parsed = nettest::parseArguments(args);

        if (!parsed.ok || parsed.settings.help) {
            return std::nullopt;
        }

        if (const auto exitCode = nettest::createIsolate(parsed.settings, err); exitCode != 0) {
            return exitCode;
        }

        return std::nullopt;
    }

    /// Runs the tool with the arguments after its name. A command line error: the first line of `err` is
    /// `NetTest: <text>`, the code is 2. No arguments: the usage on `out`, the code 2. --help: the usage on `out`, the
    /// code 0.
    static int runProcess(const std::vector<std::string> &args, std::ostream &out, std::ostream &err) {
        if (args.empty()) {
            out << nettest::usageText();

            return EXIT_BAD_ARGUMENTS;
        }

        const auto parsed = nettest::parseArguments(args);

        if (!parsed.ok) {
            err << "NetTest: " << parsed.error << "\n\n" << nettest::usageText();

            return EXIT_BAD_ARGUMENTS;
        }

        if (parsed.settings.help) {
            out << nettest::usageText();

            return 0;
        }

        return nettest::run(parsed.settings, out, err);
    }
};

} // namespace dxfcpp::tools
