// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../Args/Args.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <range/v3/all.hpp>

namespace dxfcpp::tools {

struct HelpTool {
    static const std::unordered_map<std::string, std::string>
        EMBEDDED_ARTICLES;
    static const std::string NAME;
    static const std::string SHORT_DESCRIPTION;
    static const std::string DESCRIPTION;
    static const std::vector<std::string> USAGE;
    static const std::vector<std::string> ADDITIONAL_INFO;

    template <typename Args> void run(Args &&args) {
        using namespace std::literals;
    }
};

} // namespace dxfcpp::tools