// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "ConnectTool.hpp"

namespace dxfcpp::tools {
const std::string ConnectTool::NAME{"Connect"};
const std::string ConnectTool::SHORT_DESCRIPTION{"Connects to specified address(es)."};
const std::string ConnectTool::DESCRIPTION{R"(
Connects to the specified address(es) and subscribes to the specified symbols.
)"};
const std::vector<std::string> ConnectTool::USAGE{
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> ConnectTool::ADDITIONAL_INFO{};

const std::vector<ArgType> ConnectTool::ARGS{AddressArgRequired{}, TypesArgRequired{}, SymbolsArgRequired{}, FromTimeArg{}, SourceArg{},
                                             PropertiesArg{},      TapeArg{},  QuiteArg{},   HelpArg{}};
} // namespace dxfcpp::tools