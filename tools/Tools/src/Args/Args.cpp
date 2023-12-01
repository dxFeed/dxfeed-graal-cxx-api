// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Args.hpp"

namespace dxfcpp::tools {

const std::string AddressArg::NAME{"address"};
const std::size_t AddressArg::POSITION{0};
const std::string AddressArg::HELP_TEXT{R"(
The address(es) to connect to retrieve data (see "Help address").
For Token-Based Authorization, use the following format: "<address>:<port>[login=entitle:<token>]".
)"};

const std::string TypesArg::NAME{"types"};
const std::size_t TypesArg::POSITION{1};
const std::string TypesArg::HELP_TEXT{R"(
Comma-separated list of dxfeed event types (e.g. Quote, TimeAndSale).
Use "all" for all available event types.
)"};

const std::string SymbolsArg::NAME{"symbols"};
const std::size_t SymbolsArg::POSITION{2};
const std::string SymbolsArg::HELP_TEXT{R"(
Comma-separated list of symbol names to get events for (e.g. "IBM, AAPL, MSFT").
Use "all" for wildcard subscription. The "dxfeed.wildcard.enable" property must be set to true to enable wildcard subscription.
A symbol filter set format can also be used as symbols source, for example: "ipf[https://demo:demo@tools.dxfeed.com/ipf?TYPE=STOCK]"
)"};

const std::string PropertiesArg::NAME{"properties"};
const std::string PropertiesArg::SHORT_NAME{"p"};
const std::string PropertiesArg::LONG_NAME{"properties"};
const std::string PropertiesArg::HELP_TEXT{R"(
Comma-separated list of properties (key-value pair separated by an equals sign).
)"};

const std::string FromTimeArg::NAME{"fromTime"};
const std::string FromTimeArg::SHORT_NAME{"f"};
const std::string FromTimeArg::LONG_NAME{"from-time"};
const std::string FromTimeArg::HELP_TEXT{R"(
From-time for the history subscription in standard formats (see "Help Time Format").
)"};

const std::string SourceArg::NAME{"source"};
const std::string SourceArg::SHORT_NAME{"s"};
const std::string SourceArg::LONG_NAME{"source"};
const std::string SourceArg::HELP_TEXT{R"(
Order source for the indexed subscription (e.g. NTV, ntv).
)"};

const std::string TapeArg::NAME{"tape"};
const std::string TapeArg::SHORT_NAME{"t"};
const std::string TapeArg::LONG_NAME{"tape"};
const std::string TapeArg::HELP_TEXT{R"(
Tape all incoming data into the specified file (see "Help Tape").
)"};

const std::string QuiteArg::NAME{"quite"};
const std::string QuiteArg::SHORT_NAME{"q"};
const std::string QuiteArg::LONG_NAME{"quite"};
const std::string QuiteArg::HELP_TEXT{R"(
Be quiet, event printing is disabled.
)"};

const std::string ForceStreamArg::NAME{"forceStream"};
const std::string ForceStreamArg::SHORT_NAME{};
const std::string ForceStreamArg::LONG_NAME{"force-stream"};
const std::string ForceStreamArg::HELP_TEXT{R"(
Enforces a streaming contract for subscription. The StreamFeed role is used instead of Feed.
)"};

const std::string CPUUsageByCoreArg::NAME{"cpuUsageByCore"};
const std::string CPUUsageByCoreArg::SHORT_NAME{};
const std::string CPUUsageByCoreArg::LONG_NAME{"cpu-usage-by-core"};
const std::string CPUUsageByCoreArg::HELP_TEXT{R"(
Show CPU usage by core (where 1 core = 100%).
)"};

const std::string DetachListenerArg::NAME{"tape"};
const std::string DetachListenerArg::SHORT_NAME{};
const std::string DetachListenerArg::LONG_NAME{"detach-listener"};
const std::string DetachListenerArg::HELP_TEXT{R"(
Don't attach a listener. Used for debugging purposes.
)"};

const std::string IntervalArg::NAME{"interval"};
const std::string IntervalArg::SHORT_NAME{};
const std::string IntervalArg::LONG_NAME{"interval"};
const std::string IntervalArg::HELP_TEXT{R"(
Measurement interval in seconds.
)"};

const std::string HelpArg::NAME{"help"};
const std::string HelpArg::SHORT_NAME{"h"};
const std::string HelpArg::LONG_NAME{"help"};
const std::string HelpArg::HELP_TEXT{R"(
Display this help screen.
)"};

const std::string ArticleArgRequired::NAME{"article"};
const std::size_t ArticleArgRequired::POSITION{0};
const std::string ArticleArgRequired::HELP_TEXT{R"(
Help article to show.
)"};

}