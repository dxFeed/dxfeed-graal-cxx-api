// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "DumpTool.hpp"

namespace dxfcpp::tools {
const std::string DumpTool::NAME{"Dump"};
const std::string DumpTool::SHORT_DESCRIPTION{"Dumps all events received from address."};
const std::string DumpTool::DESCRIPTION{R"(
Dumps all events received from address.
Enforces a streaming contract for subscription. A wildcard enabled by default.
This was designed to receive data from a file.
If <types> is not specified, creates a subscription for all available event types.
If <symbol> is not specified, the wildcard symbol is used.
)"};
const std::vector<std::string> DumpTool::USAGE{
    NAME + " <address> [<options>]",
    NAME + " <address> <types> [<options>]",
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> DumpTool::ADDITIONAL_INFO{};
} // namespace dxfcpp::tools