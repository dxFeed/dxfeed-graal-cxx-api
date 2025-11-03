// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Help/HelpTool.hpp"
#include <variant>
#include <fmt/std.h>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
DXFCXX_DISABLE_CLANG_WARNINGS_PUSH("-Wdeprecated-declarations")
DXFCXX_DISABLE_CLANG_WARNINGS_POP()
DXFCXX_DISABLE_MSC_WARNINGS_POP()

namespace dxfcpp::tools {

struct Tools {
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;
    static const std::vector<HelpTool::Tool> ARGS;

    [[nodiscard]] static std::string getName() noexcept {
        return "Tools " DXFCXX_VERSION;
    }

    [[nodiscard]] static std::string getFullName() noexcept {
        return "Tools " DXFCXX_VERSION;
    }
};

}