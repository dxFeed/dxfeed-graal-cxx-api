// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Tools.hpp"

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

const std::vector<ArgType> ConnectTool::ARGS{
    AddressArgRequired{}, TypesArgRequired{}, SymbolsArgRequired{}, FromTimeArg{},    SourceArg{},
    PropertiesArg{},      TapeArg{},          QuiteArg{},           ForceStreamArg{}, HelpArg{}};

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

const std::vector<ArgType> DumpTool::ARGS{AddressArgRequired{}, TypesArg{}, SymbolsArg{}, PropertiesArg{}, TapeArg{},
                                          QuiteArg{},           HelpArg{}};

const std::string LatencyTest::NAME{"LatencyTest"};
const std::string LatencyTest::SHORT_DESCRIPTION{"Connects to the specified address(es) and calculates latency."};
const std::string LatencyTest::DESCRIPTION{R"(
Connects to the specified address(es) and calculates latency.
)"};
const std::vector<std::string> LatencyTest::USAGE{
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> LatencyTest::ADDITIONAL_INFO{};

const std::vector<ArgType> LatencyTest::ARGS{AddressArgRequired{}, TypesArg{},    SymbolsArg{}, PropertiesArg{},
                                             ForceStreamArg{},     IntervalArg{}, HelpArg{}};

const std::string PerfTestTool::NAME{"PerfTest"};
const std::string PerfTestTool::SHORT_DESCRIPTION{"Connects to specified address and calculates performance counters."};
const std::string PerfTestTool::DESCRIPTION{R"(
Connects to the specified address(es) and calculates performance counters (events per second, cpu usage, etc).
)"};
const std::vector<std::string> PerfTestTool::USAGE{
    NAME + " <address> <types> <symbols> [<options>]",
};
const std::vector<std::string> PerfTestTool::ADDITIONAL_INFO{};

const std::vector<ArgType> PerfTestTool::ARGS{AddressArgRequired{}, TypesArgRequired{}, SymbolsArgRequired{},
                                              PropertiesArg{},      ForceStreamArg{},   CPUUsageByCoreArg{},
                                              DetachListenerArg{},  HelpArg{}};

const std::string QdsTool::NAME{"Qds"};
const std::string QdsTool::SHORT_DESCRIPTION{"A collection of tools ported from the Java qds-tools."};
const std::string QdsTool::DESCRIPTION{R"(
A collection of tools ported from the Java qds-tools.
)"};
const std::vector<std::string> QdsTool::USAGE{
    NAME + " \"<qds-args>\" [<options>]",
};
const std::vector<std::string> QdsTool::ADDITIONAL_INFO{};

const std::vector<ArgType> QdsTool::ARGS{QdsArgs{}, PropertiesArg{}, HelpArg{}};

const std::unordered_map<std::string, std::string> HelpTool::EMBEDDED_ARTICLES{
    {"Connect",
     R"(This tool is used to connect to some address with specified subscription, and log or tape received data. By default it
just logs all received records to a screen in text format.

The main extra-functionality of the connect tool is taping data into file, which can be performed using --tape option.
(see "Help Tape" for its detailed description and samples).

Examples:

Connects to the demo-endpoint and subscribes to Quote event for AAPL,IBM,MSFT symbols:
  connect demo.dxfeed.com:7300 Quote AAPL,IBM,MSFT

Connects to the demo-endpoint and subscribes to Quote,Trade events for AAPL,MSFT symbols with aggregation period 5s:
  connect demo.dxfeed.com:7300 Quote,Trade AAPL,MSFT -p dxfeed.aggregationPeriod=5s

Connects to the demo-endpoint and subscribes to Order event for AAPL symbol with order source NTV (NASDAQ Total View):
  connect demo.dxfeed.com:7300 Order AAPL -s NTV

Connects to the demo-endpoint and subscribes to TimeAndSale event for AAPL symbol from starting January 1, 1970:
  connect demo.dxfeed.com:7300 TimeAndSale AAPL --from-time 0

Connects to the tape file and subscribes for all events and all symbols:
  connect file:tape.csv all all -p dxfeed.wildcard.enable=true

Connects to the demo-endpoint by dxLink and subscribes to Quote event for AAPL symbol:
  connect "dxlink:wss://demo.dxfeed.com/dxlink-ws" Quote AAPL -p dxfeed.experimental.dxlink.enable=true,scheme=ext:opt:sysprops,resource:dxlink.xml)"},
    {"Dump",
     R"(This tool dumps all received data records and subscription items to a console. It can connect to any address including
plain file.

Examples:

Dump all events and all symbols from the specified file:
  dump tape.csv

Dump all events and all symbols from the specified file at maximum speed:
  dump tape.csv[speed=max]

Dump all events and all symbols from the specified file at maximum speed, cyclically:
  dump tape.csv[speed=max,cycle]

Dump only Quote event for all symbols from the specified file:
  dump tape.csv Quote

Dump only Quote event for AAPL symbol from the specified file:
  dump tape.csv Quote AAPL

Dump only Quote event for AAPL symbol from the specified address in a stream contract:
  dump demo.dxfeed.com:7300 Quote AAPL

Tape only Quote event for AAPL symbol from the specified address in a stream contract into the specified tape file:
  dump demo.dxfeed.com:7300 Quote AAPL -q -t tape.bin)"},
    {"PerfTest", R"(Examples:

Connects to the local endpoint, subscribes to TimeAndSale event for ETH/USD:GDAX symbol and print performance counters:
  perftest 127.0.0.1:7777 TimeAndSale ETH/USD:GDAX

Connects to the tape file (on max speed and cyclically), subscribes for all symbols and print performance counters:
  perftest file:tape.csv[speed=max,cycle] all all --force-stream)"},
    {"Address",
     R"(To create a connection using any tool one must specify an address. Depending on an address format different message
connectors are used to establish connection.

To establish multiple simultaneous connections multiple addresses can be specified like:
(<address1>)(<address2>)...(<addressN>)

Every single address has the following format:
[<filter>@]<address-itself>[<connector-properties>].

<filer> is used to configure data or subscription filter in message adapter factory.

<connector-properties> is the list of initial properties passed to a connector. They have a form of comma-separated list
of pairs <key>=<value> enclosed in square [...] or round (...) brackets. Multiple pairs of brackets are also allowed, so
[key1=val1,key2=val2] has the same meaning as [key1=val1][key2=val2].

For Token-Based Authorization use "login=entitle:" property:
<address>[login=entitle:<token>]

<address-itself> format depends on particular connector.

Available connectors are:
[name]        [address format]  [description]
ClientSocket  <host>:<port>     Connects to some host using TCP/IP client socket.
ServerSocket  :<port>           Creates server TCP/IP socket connection.
NioServer     nio:<port>        TCP/IP server socket connector with scalable non-blocking API.
File          file:<filename>   Connects to a file.
Tape          tape:<filename>   Writes data to tape files.

To get detailed information about address format and available properties use "Help <connector name>".)"},
    {"ClientSocket", R"(Connects to some host using TCP/IP client socket.

Address format: <host>:<port>

Properties:
[type]        [name]          [description]
ConnectOrder  connectOrder    Order of considering specified server addresses during connect/reconnect: "shuffle"
                              (default), "random", "ordered", "priority"
String        fieldReplacer   Field Replacers for input connection
String        name            Name of this connector
String        password        User password
String        proxyHost       HTTP proxy host name
int           proxyPort       HTTP proxy port
long          reconnectDelay  Delay between reconnection attempts in milliseconds
int           threadPriority  Priority for threads associated with this connector
String        user            User login name)"},
    {"ServerSocket", R"(Creates server TCP/IP socket connection.

Address format: :<port>

Properties:
[type]  [name]          [description]
String  bindAddr        Network interface address to bind socket to
String  fieldReplacer   Field Replacers for input connection
int     maxConnections  Max number of connections allowed for connector
String  name            Name of this connector
String  password        User password
long    reconnectDelay  Delay between reconnection attempts in milliseconds
int     threadPriority  Priority for threads associated with this connector
String  user            User login name)"},
    {"NioServer", R"(TCP/IP server socket connector with scalable non-blocking API.

Address format: nio:<port>

Properties:
[type]  [name]          [description]
String  bindAddr        Network interface address to bind socket to
String  fieldReplacer   Field Replacers for input connection
int     maxConnections  Max number of connections allowed for connector
String  name            Name of this connector
String  password        User password
int     readerThreads   Number of reader threads in the pool
long    reconnectDelay  Delay between reconnection attempts in milliseconds
int     socketTimeout   SO_TIMEOUT option value
int     threadPriority  Priority for threads associated with this connector
String  user            User login name
int     writerThreads   Number of writer threads in a pool)"},
    {"File", R"(Connects to a file.

Address format: file:<filename>

Properties:
[type]             [name]          [description]
StreamCompression  compression     File compression (one of "none", "gzip", or "zip"), autodetect by default from
                                   file header
boolean            cycle           Enables cycle playback
TimePeriod         delayed         Delay relatively to current time
String             fieldReplacer   Field Replacers for input connection
FileFormat         format          File format (one of "binary", "text", "csv", or "blob:<record>:<symbol>"),
                                   autodetect by default from file header
boolean            ignoreTime      Ignores ".time" files even if they present
String             name            Name of this connector
String             password        User password
MessageType        readAs          Overrides the type of read messages (one of "ticker_data", "stream_data",
                                   "history_data", or "raw_data", works for binary tape files only)
long               reconnectDelay  Delay between reconnection attempts in milliseconds
MessageType        resyncOn        Message type to resync partial binary stream captured with tcpdump
boolean            schemeKnown     Enables parsing of files without record descriptions
double             speed           Replay speed vs real time, use "max" to read file as fast as possible, defaults to 1
Date               start           Time to start playing from, use [YYYYMMDD-]HHMMSS[.sss][tz] format
Date               stop            Time to stop playing, use [YYYYMMDD-]HHMMSS[.sss][tz] format
int                threadPriority  Priority for threads associated with this connector
TimestampsType     time            Time format (one of "none", "long", "text", "field", or "message"), autodetect by
                                   default
String             user            User login name

File connector receives a URL of a file as an address and connects to it. It supports the same file formats as --tape
option of Connect tool (see "Help Tape") and they are usually combined.

It can read and give out all the data presented in a file at once, or it can also use ".time" files to reproduce
original delays. By default it uses these files if they present, but this can be disabled by setting "ignoretime=true".

File connector also supports reading multiple timestamped files. If filename contains special '~' marker, then it is
replaced with a timestamp. For example, address "file:records~.dat" means that we want to seek for files like
records20080320-154914+0030.dat and connect to them consecutively. Corresponding ".time" files are also used in that
case (if they exist).)"},
    {"Tape", R"(The --tape (-t) option of tools is used to tape all incoming data into the specified file.

This option has several parameters:

"format" parameter defines format of stored data. Its value can be one of "text", "binary" or
"blob:<record>:<symbol>records>" (binary format is used by default). Blob is a special format that is
used for compact store of a single-record, single-symbol data stream.

"saveas" parameter overrides the type of stored messages. Data messages can be stored as "ticker_data", "stream_data",
"history_data", or "raw_data".

"split" parameter can be used to create multiple files with timestamped names. Its value is time period (see "Help time
format") determining how often must new files be created. When this parameter is defined a special '~' marker in file
name must be used (it will be replaced with file timestamp).

"time" parameter defines whether to create ".time" files. These files store information about when each piece of data
was recorded and are used to read data from file in a realtime mode (keeping original time delays). ".time" file format
is simple: it contains lines with records of kind "<timestamp>:<file-position>". The "time" parameter can be set into
one of "none", "long" or "text". In the first case no ".time" files are created. Otherwise the value defines whether to
write timestamps in ".time" files in old-style format (single long value in milliseconds) or in human-readable text
format (see "Help time format"). By default ".time" files with old-style timestamps are created only if the file has
".data" extension.

Two special parameters "storagesize" and "storagetime" are used to drop out-of-date taped files.

"storagetime" parameter value is a time-period (see "Help Time format"). If this parameter is defined then all taped
files which have timestamps less than current time minus "storagetime" value are deleted.

"storagesize" parameter is a number followed by one-character suffix 'b' (bytes), 'k' (kilobytes), 'm' (megabytes), 'g'
(gigabytes). If the suffix is omitted then value in bytes is supposed. If this parameter is defined then the total size
of all existing taped files (not counting corresponding ".time" files) is kept at value not much greater than
"storagesize" value (also by means of deleting older files).

Both these parameters guarantee that at least specified amount of data is always stored, i.e. they define low boundary
of stored data size. These two parameters can be useful if we are implementing data delay by taping data on disk.

Note, that all files that match specified timestamped-file pattern are searched to be deleted when using these
parameters (i.e. even if these files weren't actually taped by this execution of connect tool).

Examples:

--tape quotes.log
will tape data into file "quotes.log" in binary format. No ".time" files will be created.

--tape quotes.log[saveas=ticker_data]
will tape data into file "quotes.log" with "ticker_data" messages regardless of the orignal type of the incoming data
messages.

--tape log[time=long]
will tape data into file "log" and timestamps into file "log.time" as long values (old-style).

--tape log_~.txt[format=text,split=2m,time=text]
will tape data in text format. It will create files like
log_20071114-154214+0030.txt
log_20071114-154214+0030.time
log_20071114-154400+0300.txt
log_20071114-154400+0300.time
log_20071114-154600+0300.txt
log_20071114-154600+0300.time
...
creating new pair of ".txt" and ".time" files every 2 minutes.

--tape log_~.dat[split=2m,time=text,storagesize=100k,storagetime=30m]
will act similar to a previous sample (except it will create ".dat" files and store data in binary format). Moreover it
will keep only files taped during last 30 minutes and total size of these files will be not much more than 100 kb.)"},
    {"Time Format",
     R"(Many tools handle dates, times and periods as values of their arguments, options or some input or output values. A
common flexible format exists for parsing and formatting date-times and time-periods and it is used almost everywhere.

Dates and times can be parsed from one of the following forms:

<value-in-milliseconds>
  This is a standart java representation of time as a single long number. Value of msecs is measured from 1970-01-01.
  Here the value should be positive and have at least 9 digits (otherwise it could not be distinguished from date in
  format 'yyyyMMdd'). Each date since 1970-01-03 can be represented in this form.

<date>[<time>][<timezone>]
  This is the most formal way to represent a date-time. If time is missing it is supposed to be '00:00:00'.
  If no time zone is specified in the parsed string, the string is assumed to denote a local time.

Here <date> is one of:
  yyyy-MM-dd
  yyyyMMdd

<time> is one of:
  HH:mm:ss[.sss]
  HHmmss[.sss]

<timezone> is one of:
  [+-]HH:mm
  [+-]HHmm
  Z for UTC.

Examples of valid date-times:
  20070101-123456
  20070101-123456.123
  2005-12-31 21:00:00
  2005-12-31 21:00:00.123+03:00
  2005-12-31 21:00:00.123+0400
  2007-11-02Z
  123456789 // value-in-milliseconds)"},
};

const std::string HelpTool::NAME{"Help"};
const std::string HelpTool::SHORT_DESCRIPTION{"Help tool."};
const std::string HelpTool::DESCRIPTION{R"(
Displays documentation pages.
)"};
const std::vector<std::string> HelpTool::USAGE{
    NAME + " <article>",
    NAME + " contents",
    NAME + " all",
};
const std::vector<std::string> HelpTool::ADDITIONAL_INFO{
    R"(To see help on some topic type "Help <topic>".)",
    R"(To see list of all articles type "Help contents".)",
    R"(Use "Help all" to generate all existing help articles.)",
};

const std::vector<ArgType> HelpTool::ARGS{ArticleArgRequired{}, HelpArg{}};

const std::vector<std::string> HelpTool::ALL_ARTICLE_NAMES =
    EMBEDDED_ARTICLES | ranges::views::keys | ranges::to<std::vector<std::string>>();

const std::unordered_map<std::string, tools::HelpTool::Tool> tools::HelpTool::ALL_TOOLS{
    {ConnectTool::getName(), ConnectTool{}},   {DumpTool::getName(), DumpTool{}},
    {HelpTool::getName(), HelpTool{}},         {LatencyTest::getName(), LatencyTest{}},
    {PerfTestTool::getName(), PerfTestTool{}}, {QdsTool::getName(), QdsTool{}},
};

const std::vector<std::string> tools::HelpTool::ALL_TOOL_NAMES =
    tools::HelpTool::ALL_TOOLS | ranges::views::keys | ranges::to<std::vector<std::string>>();

const std::set<std::string> HelpTool::ALL_NAMES =
    ranges::views::concat(ALL_ARTICLE_NAMES, ALL_TOOL_NAMES) | ranges::to<std::set>;

const std::string Tools::NAME{"Tools"};
const std::string Tools::SHORT_DESCRIPTION{"A collection of useful utilities that use the dxFeed API."};
const std::string Tools::DESCRIPTION{};
const std::vector<std::string> Tools::USAGE{
    NAME + " <tool> [...]",
};
const std::vector<std::string> Tools::ADDITIONAL_INFO{
    {R"(To get detailed help on some tool use "Help <tool>".)"},
};

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Waggressive-loop-optimizations")
const std::vector<tools::HelpTool::Tool> Tools::ARGS{tools::HelpTool::ALL_TOOLS | ranges::views::values |
                                                     ranges::to<std::vector>};
DXFCXX_DISABLE_GCC_WARNINGS_POP()
} // namespace dxfcpp::tools