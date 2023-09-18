// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Args.hpp"

namespace dxfcpp::tools {

const std::string AddressArg::NAME{"address"};
const std::string AddressArg::HELP_TEXT{R"(
The address(es) to connect to retrieve data (see "Help address").
For Token-Based Authorization, use the following format: "<address>:<port>[login=entitle:<token>]".
)"};

const std::string TypesArg::NAME{"types"};
const std::string TypesArg::HELP_TEXT{R"(
Comma-separated list of dxfeed event types (e.g. Quote, TimeAndSale).
Use "all" for all available event types.
)"};

const std::string SymbolsArg::NAME{"symbols"};
const std::string SymbolsArg::HELP_TEXT{R"(
Comma-separated list of symbol names to get events for (e.g. "IBM, AAPL, MSFT").
Use "all" for wildcard subscription.
The "dxfeed.wildcard.enable" property must be set to true to enable wildcard subscription.
)"};

}