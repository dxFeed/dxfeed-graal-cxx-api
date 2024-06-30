<picture>
 <source media="(prefers-color-scheme: dark)" srcset="docs/images/logo_dark.svg">
 <img alt="light" src="docs/images/logo_light.svg">
</picture>

This package provides access to [dxFeed market data](https://dxfeed.com/market-data/).
The library is built as a language-specific wrapper over
the [dxFeed Graal Native](https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-api/) library,
which was compiled with [GraalVM Native Image](https://www.graalvm.org/latest/reference-manual/native-image/)
and [dxFeed Java API](https://docs.dxfeed.com/dxfeed/api/overview-summary.html) (our flagman API).

:information_source: If you already use [dxFeed C API](https://github.com/dxFeed/dxfeed-c-api), please see
the [Overview](#overview) section.<br>

[![Build](https://github.com/dxFeed/dxfeed-graal-cxx-api/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/dxFeed/dxfeed-graal-cxx-api/actions/workflows/build.yml)
![Platform](https://img.shields.io/badge/platform-win--x64%20%7C%20linux--x64%20%7C%20osx--x64%20%7C%20osx--aarch64-lightgrey)
![](https://img.shields.io/badge/C++%20standard-C++20-blueviolet) ![](https://img.shields.io/badge/C%20standard-C11-blueviolet)
[![Release](https://img.shields.io/github/v/release/dxFeed/dxfeed-graal-cxx-api)](https://github.com/dxFeed/dxfeed-graal-cxx-api/releases/latest)
[![License](https://img.shields.io/badge/license-MPL--2.0-orange)](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/master/LICENSE)
<!-- [![Downloads](https://img.shields.io/github/downloads/dxFeed/dxfeed-graal-cxx-api/total)](https://github.com/dxFeed/dxfeed-graal-cxx-api/releases/latest) -->

## Table of Contents

- [Overview](#overview)
  * [Reasons for the New C/C++ API Repository](#reasons-for-the-new-cxx-api-repository)
  * [Benefits of the New Version](#benefits-of-the-new-version)
  * [Milestones](#milestones)
  * [Migration](#migration)
  * [Implementation Details](#implementation-details)
  * [Architectural Restrictions and Other Limitations in the Old Version](#architectural-restrictions-and-other-limitations-of-the-old-version)
- [Documentation](#documentation)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
  * [How to connect to QD endpoint](#how-to-connect-to-qd-endpoint)
  * [How to connect to dxLink](#how-to-connect-to-dxlink)
- [Tools](#tools)
- [Samples](#samples)
- [Current State](#current-state)
- [Dependencies](DEPENDENCIES.md)
- [How To Build](HOW_TO_BUILD.md)
- [3rd Party Licenses](THIRD_PARTY_LICENSES.md)

## Overview

### Reasons for the New CXX API Repository

The old version of [dxFeed C API](https://github.com/dxFeed/dxfeed-c-api), has
several [architectural restrictions](#architectural-restrictions-and-other-limitations-of-the-old-version)
that prevent us from providing a state-of-the-art technological solution.

### Benefits of the New Version

- :rocket: Increased performance
- :milky_way: Wider functionality
- :gemini: Identical programming interfaces to our best API
- :thumbsup: Higher quality of support and service

### Milestones

Feature development has already stopped for the [old version](https://github.com/dxFeed/dxfeed-c-api) of dxFeed C
API.

The old version will be considered deprecated, and at the end of 2024, we plan to end the service. If you’re already our customer and have difficulty with a future transition, please contact us via our customer portal.

### Migration

To help you rewrite the existing API calls, we’ve prepared [samples](#samples) demonstrating how to work with the new
API and how several functionalities are implemented. More examples will follow. The table below shows the sample mapping
between the old and new versions.

Our support team on our [customer portal](https://jira.in.devexperts.com/servicedesk/customer/portal/1/create/122) is
ready to answer any questions and help with the transition.

#### Sample Mapping

| # | Sample                                                                                            | Old Version                                                                                                     | New Version                                              |
|:-:|:--------------------------------------------------------------------------------------------------|:----------------------------------------------------------------------------------------------------------------|:---------------------------------------------------------|
| 1 | How to subscribe to `Order`, `SpreadOrder`, `Candle`, `TimeAndSale`, `Greeks`, `Series` snapshots | [SnapshotConsoleSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/SnapshotConsoleSample)       | *Q2’2024*, please see [TBD](#future-development) section |
| 2 | How to subscribe to depth of market                                                               | [PriceLevelBookSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/PriceLevelBookSample)         | *Q2’2024*, please see [TBD](#future-development) section |
| 3 | How to subscribe to order snapshot with incremental updates                                       | [IncSnapshotConsoleSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/IncSnapshotConsoleSample) | *Q2’2024*, please see [TBD](#future-development) section |
| 4 | How to get and display the full order book                                                        | [FullOrderBookSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/FullOrderBookSample)           | *Q2’2024*, please see [TBD](#future-development) section |
| 5 | How to access and visualize regional market data                                                  | [RegionalBookSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/RegionalBookSample)             | *Q2’2024*, please see [TBD](#future-development) section |

### Implementation Details

We use [GraalVM Native Image](https://www.graalvm.org/latest/reference-manual/native-image/) technology and specially
written code that *wraps* Java methods into native ones
to get dynamically linked libraries for different platforms (Linux, macOS, and Windows) based on
the [latest Java API package](https://dxfeed.jfrog.io/artifactory/maven-open/com/devexperts/qd/dxfeed-api/).

Then, the resulting dynamic link library (dxFeed Graal-native) is used through
C [ABI](https://en.wikipedia.org/wiki/Application_binary_interface) (application binary interface),
and we write programming interfaces that describe our business model (similar to Java API).

As a result, we get a full-featured, similar performance as with Java API.
Regardless of the language, writing the final application logic using API calls will be very similar (only the syntax
will be amended, *"best practices"*, specific language restrictions).

Below is a scheme of this process:

<picture>
 <source media="(prefers-color-scheme: dark)" srcset="docs/images/scheme_dark.svg">
 <img alt="light" src="docs/images/scheme_light.svg">
</picture>

### Architectural Restrictions and Other Limitations of the Old Version

| # | Limitation                                                                                                                                                                                                                                                                                                                                         | How It’s Solved in the New Version                                                                                                                                                                                                                                                       |
|:-:|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | Single-threaded architecture limiting throughput                                                                                                                                                                                                                                                                                                   | Based on the Java API, each subscription object ([DXFeedSubscription](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeedSubscription.html)) *can* run on its own thread                                                                                                            |
| 2 | User code in event callbacks (for example, [dxf_event_listener_t](https://docs.dxfeed.com/c-api/group__event-data-structures-event-subscription-stuff.html#gac8bcb70cd4c8857f286f4be65e9522c6)) is executed in the socket read thread, which can significantly reduce throughput                                                                   | Socket processing threads and callback threads are separated                                                                                                                                                                                                                             |
| 3 | In event callbacks, one market event type and one data portion always arrive (excluding snapshot subscription), which increases the load on the CPU with a large amount of incoming data                                                                                                                                                           | Event callbacks can receive different market event types, and more than one by batch                                                                                                                                                                                                     |
| 4 | It’s impossible to subscribe to data without getting [regionals](https://kb.dxfeed.com/en/data-model/exchange-codes.html) (if it is available for the market event) or only for a certain regional                                                                                                                                                 | `subscription->addSymbols({"AAPL"});` - [composite](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#quote-47603)<br>`subscription->addSymbols({"AAPL&Q"});` - [regional](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#quote-x--regional-quote-) |
| 5 | It’s impossible to subscribe to `Order` event (excluding snapshot subscription) without getting: all [sources](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#order-x), `Order` by `Quote` (including regionals), Order by [MarketMaker](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#marketmaker-47603) | `subscription->addSymbols(IndexedEventSubscriptionSymbol::create("AAPL", OrderSource::NTV));` - [OrderSource](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/include/dxfeed_graal_native_cpp_api/event/market/OrderSource.hpp) determines which data is being subscribed to    |
| 6 | Data is mixed up when creating two subscriptions (regular and time series) for the same market event type. Both regular and time series data go to both subscriptions                                                                                                                                                                              | Each subscription instance receives only the data requested                                                                                                                                                                                                                              |
| 7 | Each subsequent request for the same symbol set in a subscription instance overwrites the existing one in another subscription instance                                                                                                                                                                                                            | Subscription instances and the data they receive are independent of each other                                                                                                                                                                                                           |
| 8 | Removing a symbol from one subscription instance caused it to be removed from all others                                                                                                                                                                                                                                                           | Subscription instances and the data they receive are independent of each other                                                                                                                                                                                                           |
| 9 | Incorrect behavior when reading from a file (if a market event in the file hasn\'t been subscribed to). Reading from a file always occurs at maximum speed. The supported format is binary only                                                                                                                                                    | `endpoint->connect("file:tape.txt[format=text]");` - processing a text file with at it's "real" speed by timestamps<br>`endpoint->connect("file:tape.bin[format=binary,speed=max]");` - processing a binary file with max speed                                                          |

## Documentation

Find useful information in our self-service dxFeed Knowledge Base or CXX API documentation:

- [dxFeed Graal C/C++ API documentation](https://dxfeed.github.io/dxfeed-graal-cxx-api/)
- [dxFeed Knowledge Base](https://kb.dxfeed.com/index.html?lang=en)
  * [Getting Started](https://kb.dxfeed.com/en/getting-started.html)
  * [Troubleshooting](https://kb.dxfeed.com/en/troubleshooting-guidelines.html)
  * [Market Events](https://kb.dxfeed.com/en/data-model/dxfeed-api-market-events.html)
  * [Event Delivery contracts](https://kb.dxfeed.com/en/data-model/model-of-event-publishing.html#event-delivery-contracts)
  * [dxFeed API Event classes](https://kb.dxfeed.com/en/data-model/model-of-event-publishing.html#dxfeed-api-event-classes)
  * [Exchange Codes](https://kb.dxfeed.com/en/data-model/exchange-codes.html)
  * [Order Sources](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#order-x)
  * [Order Book reconstruction](https://kb.dxfeed.com/en/data-model/dxfeed-order-book/order-book-reconstruction.html)
  * [Symbology Guide](https://kb.dxfeed.com/en/data-model/symbology-guide.html)

## Requirements

### Windows

Only x64 versions are supported.

| OS                                    | Version        | Architectures |
|---------------------------------------|----------------|---------------|
| [Windows][Windows-client]             | 8, 8.1         | x64           |
| [Windows 10][Windows-client]          | Version 1607+  | x64           |
| [Windows 11][Windows-client]          | Version 22000+ | x64           |
| [Windows Server][Windows-Server]      | 2012+          | x64           |
| [Windows Server Core][Windows-Server] | 2012+          | x64           |
| [Nano Server][Nano-Server]            | Version 1809+  | x64           |

#### Requirements

* [Visual C++ Redistributable for Visual Studio 2015][vc_redist]

[Windows-client]: https://www.microsoft.com/windows/

[Windows-Server]: https://learn.microsoft.com/windows-server/

[Nano-Server]: https://learn.microsoft.com/windows-server/get-started/getting-started-with-nano-server

[vc_redist]: https://aka.ms/vs/17/release/vc_redist.x64.exe

### Linux

Only x64 versions are supported.

#### libc compatibility

- [glibc][glibc]: 2.35+ (from Ubuntu 22.04)
- [musl][musl]: temporarily unsupported

#### libpthread compatibility

A symlink on `libpthread.so`, `libpthread.so.0`, or `libcoreclr.so` must exist.

[glibc]: https://www.gnu.org/software/libc/

[musl]: https://musl.libc.org/

### macOS

| OS             | Version | Architectures |
|----------------|---------|---------------|
| [macOS][macOS] | 10.15+  | x64           |
| [macOS][macOS] | 11+     | Arm64         |

[macOS]: https://support.apple.com/macos

## Installation

Examples of how to install the library and integrate with the library can be
found [here](https://github.com/dxFeed/dxfeed-graal-cxx-api-samples).

The API is delivered as a dynamic library (`dxFeedGraalCxxApi.so|dll|dylib`) and a static
one (`dxFeedGraalCxxApi_static.a|lib`).
Also, it depends on `DxFeedGraalNativeSdk.so|dll|dylib`. Please place it nearby or available on `PATH` (Or
use `RPATH`, `LD_LIBRARY_PATH`).

## Usage

### How to connect to QD endpoint

To use the dynamic library define the `DXFCPP_USE_DLLS` preprocessor directive.

```cpp
#include <iostream>
#include <dxfeed_graal_cpp_api/api.hpp>

int main() {
    using namespace dxfcpp;
    
    // For token-based authorization, use the following address format:
    // "demo.dxfeed.com:7300[login=entitle:token]"
    auto endpoint = DXEndpoint::newBuilder()
            ->withProperty("dxfeed.address", "demo.dxfeed.com:7300")
            ->build();
    
    // Another way to connect the endpoint.
    // auto endpoint = DXEndpoint::create()->connect("demo.dxfeed.com:7300");
    
    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);
    
    subscription->addEventListener([](auto&& events) {
        for (auto&& e : events) {
            std::cout << e << "\n";
        }
    });
    
    subscription->addSymbols({"AAPL"});
    
    std::cin.get();
}
```

<details>
<summary>Output</summary>
<br>

```
I 221219 224811.681 [main] QD - Using QDS-3.313+file-UNKNOWN+mars-UNKNOWN+monitoring-UNKNOWN+tools-UNKNOWN, (C) Devexperts
I 221219 224811.695 [main] QD - Using scheme com.dxfeed.api.impl.DXFeedScheme DH2FdjP0DtOEIOAbE4pRVpmJsPnaZzAo1mICPJ6b06w
I 221219 224812.010 [main] QD - qd with collectors [Ticker, Stream, History]
I 221219 224812.017 [main] ClientSocket-Distributor - Starting ClientSocketConnector to demo.dxfeed.com:7300
I 221219 224812.017 [demo.dxfeed.com:7300-Reader] ClientSocketConnector - Resolving IPs for demo.dxfeed.com
I 221219 224812.021 [demo.dxfeed.com:7300-Reader] ClientSocketConnector - Connecting to 208.93.103.170:7300
I 221219 224812.170 [demo.dxfeed.com:7300-Reader] ClientSocketConnector - Connected to 208.93.103.170:7300
D 221219 224812.319 [demo.dxfeed.com:7300-Reader] QD - Distributor received protocol descriptor multiplexor@WQMPz [type=qtp, version=QDS-3.306, opt=hs, mars.root=mdd.demo-amazon.multiplexor-demo1] sending [TICKER, STREAM, HISTORY, DATA] from 208.93.103.170
Quote{AAPL, eventTime=0, time=20221219-223311.000, timeNanoPart=0, sequence=0, bidTime=20221219-223311, bidExchange=Q, bidPrice=132.16, bidSize=2, askTime=20221219-223311, askExchange=K, askPrice=132.17, askSize=10}
Quote{AAPL, eventTime=0, time=20221219-223312.000, timeNanoPart=0, sequence=0, bidTime=20221219-223312, bidExchange=Q, bidPrice=132.16, bidSize=6, askTime=20221219-223312, askExchange=K, askPrice=132.17, askSize=10}
Quote{AAPL, eventTime=0, time=20221219-223312.000, timeNanoPart=0, sequence=0, bidTime=20221219-223312, bidExchange=K, bidPrice=132.16, bidSize=10, askTime=20221219-223312, askExchange=V, askPrice=132.17, askSize=4}
```

</details>

### How to connect to dxLink

```cpp
#include <iostream>
#include <dxfeed_graal_cpp_api/api.hpp>

int main() {
    using namespace dxfcpp;
    
    // Enable experimental feature.
    System::setProperty("dxfeed.experimental.dxlink.enable", "true");
    // Set scheme for dxLink.
    System::setProperty("scheme", "ext:opt:sysprops,resource:dxlink.xml");
    
    // For token-based authorization, use the following address format:
    // "dxlink:wss://demo.dxfeed.com/dxlink-ws[login=dxlink:token]"
    auto endpoint = DXEndpoint::newBuilder()
            ->withProperty("dxfeed.address", "dxlink:wss://demo.dxfeed.com/dxlink-ws")
            ->build();
    
    // Another way to connect the endpoint.
    // auto endpoint = DXEndpoint::create()->connect("dxlink:wss://demo.dxfeed.com/dxlink-ws");
    
    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);
    
    subscription->addEventListener([](auto&& events) {
        for (auto&& e : events) {
            std::cout << e << "\n";
        }
    });
    
    subscription->addSymbols({"AAPL"});
    
    std::cin.get();
}
```

<details>
<summary>Output</summary>
<br>

```
I 231130 154554.594 [main] QD - Using QDS-3.325+file-UNKNOWN, (C) Devexperts
I 231130 154554.602 [main] QD - Using scheme com.dxfeed.api.impl.DXFeedScheme slfwemJduh1J7ibvy9oo8DABTNhNALFQfw0KmE40CMI
I 231130 154555.094 [main] MARS - Started time synchronization tracker using multicast 239.192.51.45:5145 with gixCx
I 231130 154555.100 [main] MARS - Started JVM self-monitoring
I 231130 154555.100 [main] QD - monitoring with collectors [Ticker, Stream, History]
I 231130 154555.103 [main] QD - monitoring DXEndpoint with dxfeed.address=dxlink:wss://demo.dxfeed.com/dxlink-ws
I 231130 154555.105 [main] DxLinkClientWebSocket-Distributor - Starting DxLinkClientWebSocketConnector to wss://demo.dxfeed.com/dxlink-ws
I 231130 154555.107 [wss://demo.dxfeed.com/dxlink-ws-Writer] DxLinkClientWebSocket-Distributor - Connecting to wss://demo.dxfeed.com/dxlink-ws
I 231130 154556.640 [wss://demo.dxfeed.com/dxlink-ws-Writer] DxLinkClientWebSocket-Distributor - Connected to wss://demo.dxfeed.com/dxlink-ws
D 231130 154557.801 [oioEventLoopGroup-2-1] QD - Distributor received protocol descriptor [type=dxlink, version=0.1-0.18-20231017-133150, keepaliveTimeout=120, acceptKeepaliveTimeout=5] sending [] from wss://demo.dxfeed.com/dxlink-ws
D 231130 154557.802 [oioEventLoopGroup-2-1] QD - Distributor received protocol descriptor [type=dxlink, version=0.1-0.18-20231017-133150, keepaliveTimeout=120, acceptKeepaliveTimeout=5, authentication=] sending [] from wss://demo.dxfeed.com/dxlink-ws
Quote{AAPL, eventTime=0, time=20231130-152903.000, timeNanoPart=0, sequence=0, bidTime=20231130-152903, bidExchange=P, bidPrice=189.9, bidSize=5.0, askTime=20231130-152854, askExchange=K, askPrice=189.98, askSize=10.0}
```

</details>

To familiarize with the dxLink protocol, please click [here](https://demo.dxfeed.com/dxlink-ws/debug/#/protocol).

## Tools

[Tools](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/)
is a collection of utilities that allow you to subscribe to various market events for the specified symbols. The tools
can
be downloaded from [Release](https://github.com/dxFeed/dxfeed-graal-cxx-api/releases) (including self-contained
versions):

* [Connect](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/Connect/ConnectTool.hpp)
  connects to the specified address(es) and subscribes to the specified events with the specified symbol
* [Dump](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/Dump/DumpTool.hpp)
  dumps all events received from address. This was designed to retrieve data from a file
* [PerfTest](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/PerfTest/PerfTestTool.hpp)
  connects to the specified address(es) and calculates performance counters (events per second, memory usage, CPU usage,
  etc.)
* [LatencyTest](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/LatencyTest/LatencyTestTool.hpp)
  connects to the specified address(es) and calculates latency
* [Qds](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/Qds/QdsTool.hpp)
  collection of tools ported from the Java qds-tools

## Samples

- [x] [ConvertTapeFile](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/ConvertTapeFile/src/main.cpp)
  demonstrates how to convert one tape file to another tape file with optional intermediate processing or filtering
- [x] [DxFeedFileParser](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedFileParser/src/main.cpp)
  is a simple demonstration of how events are read form a tape file
- [x] [DxFeedSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedSample/src/main.cpp)
  is a simple demonstration of how to create multiple event listeners and subscribe to `Quote` and `Trade` events
- [x] [PrintQuoteEvents](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/PrintQuoteEvents/src/main.cpp)
  is a simple demonstration of how to subscribe to the `Quote` event, using a `DxFeed` instance singleton
  and `dxfeed.properties` file
- [x] [WriteTapeFile](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/WriteTapeFile/src/main.cpp)
  is a simple demonstration of how to write events to a tape file
- [x] [DxFeedIpfConnect](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedIpfConnect/src/main.cpp)
  is a simple demonstration of how to get Instrument Profiles
- [x] [DxFeedLiveIpfSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedLiveIpfSample/src/main.cpp)
  is a simple demonstration of how to get live updates for Instrument Profiles
- [x] [PublishProfiles](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/PublishProfiles/src/main.cpp) is a simple demonstration of how to publish market events
- [x] [ScheduleSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/ScheduleSample/src/main.cpp)
  is a simple demonstration of how to get various scheduling information for instruments
- [x] [OnDemandSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/OnDemandSample/src/main.cpp)
  a sample that demonstrates how to use the dxFeed on-demand history data replay service API
- [x] [FetchDailyCandles](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/FetchDailyCandles/src/main.cpp)
  a sample that demonstrates how to fetch last 20 days of candles for a specified symbol and print them.

## Current State

### Endpoint Roles

- [FEED](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2dae352a08d4fba713e5ae1e538bf27258c) connects to the remote data
  feed provider and is optimized for real-time or delayed data processing, this is a default role
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [STREAM_FEED](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2da08e8da2638e7a4a4dfb3d29ab5afb751) is similar to
  FEED and also connects to the remote data feed provider but is designed for bulk data parsing from files
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [PUBLISHER](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2da02c25714ac6e2973dfb530a7520c80ae) connects to the
  remote publisher hub (also known as multiplexor) or creates a publisher on the local
  host ([WriteTapeFile](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/WriteTapeFile/src/main.cpp))
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [STREAM_PUBLISHER](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2da511c10e349c1261b726f4bf830c8a0e2) is
  similar to PUBLISHER and also connects to the remote publisher hub, but is designed for bulk data publishing
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [LOCAL_HUB](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2da16722a7e83ac68f9bc32cae5242046f5) is a local hub
  without the ability to establish network connections. Events published via publisher are delivered to local feed only.
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [ON_DEMAND_FEED](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d03/structdxfcpp_1_1_d_x_endpoint.html#ac5cd7cd6eed2fdaf9daf96d4e0cf9f2da4e0e3dbd8f06b71f54c84c97fea30e22) is similar
  to FEED, but it is designed to be used with OnDemandService for historical data replay only
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API

### Event Types

- [Order](https://dxfeed.github.io/dxfeed-graal-cxx-api/d0/d00/classdxfcpp_1_1_order.html) is a snapshot of the full available
  market depth for a symbol
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [SpreadOrder](https://dxfeed.github.io/dxfeed-graal-cxx-api/da/d01/classdxfcpp_1_1_spread_order.html) is a snapshot of the
  full available market depth for all spreads
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [AnalyticOrder](https://dxfeed.github.io/dxfeed-graal-cxx-api/d4/d00/classdxfcpp_1_1_analytic_order.html) represents an
  extension of Order introducing analytic information, e.g., adding iceberg-related
  information to this order
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Trade](https://dxfeed.github.io/dxfeed-graal-cxx-api/db/d01/classdxfcpp_1_1_trade.html) is a snapshot of the price and size
  of the last trade during regular trading hours and an overall day
  volume and day turnover
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [TradeETH](https://dxfeed.github.io/dxfeed-graal-cxx-api/dd/d02/classdxfcpp_1_1_trade_e_t_h.html) is a snapshot of the price
  and size of the last trade during extended trading hours and the extended
  trading hours day volume and day turnover
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Candle](https://dxfeed.github.io/dxfeed-graal-cxx-api/de/d03/classdxfcpp_1_1_candle.html) - event with open, high, low, and
  close prices and other information for a specific period
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Quote](https://dxfeed.github.io/dxfeed-graal-cxx-api/df/d03/classdxfcpp_1_1_quote.html) is a snapshot of the best bid and
  ask prices and other fields that change with each quote
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Profile](https://dxfeed.github.io/dxfeed-graal-cxx-api/db/d00/classdxfcpp_1_1_profile.html) is a snapshot that contains the
  security instrument description
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Summary](https://dxfeed.github.io/dxfeed-graal-cxx-api/dd/d03/classdxfcpp_1_1_summary.html) is a snapshot of the trading
  session, including session highs, lows, etc.
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [TimeAndSale](https://dxfeed.github.io/dxfeed-graal-cxx-api/d3/d00/classdxfcpp_1_1_time_and_sale.html) - represents a trade or
  other market event with price, like market open/close price, etc.
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Greeks](https://dxfeed.github.io/dxfeed-graal-cxx-api/d9/d01/classdxfcpp_1_1_greeks.html) is a snapshot of the option
  price, Black-Scholes volatility, and Greeks
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Series](https://dxfeed.github.io/dxfeed-graal-cxx-api/d1/d02/classdxfcpp_1_1_series.html) is a snapshot of computed values
  available for all options series for a given underlying symbol based on options market prices
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [TheoPrice](https://dxfeed.github.io/dxfeed-graal-cxx-api/d2/d03/classdxfcpp_1_1_theo_price.html) is a snapshot of the
  theoretical option price computation that is periodically performed
  by [dxPrice](http://www.devexperts.com/en/products/price.html) model-free computation
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Underlying](https://dxfeed.github.io/dxfeed-graal-cxx-api/db/d02/classdxfcpp_1_1_underlying.html) is a snapshot of computed
  values available for an option underlying symbol based on the market’s option prices
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [Message](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/misc/Message.html) is an event with an
  application-specific attachment
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [OptionSale](https://dxfeed.github.io/dxfeed-graal-cxx-api/de/d02/classdxfcpp_1_1_option_sale.html) is an event that represents
  a trade or another market event with the price (for example, market open/close price, etc.) for each option symbol
  listed under the specified Underlying
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [OtcMarketsOrder](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/OtcMarketsOrder.html) represents an
  extension of `Order` for the symbols traded on the OTC Markets
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API

### Subscription Symbols

- String is a string representation of the symbol
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [TimeSeriesSubscriptionSymbol](https://dxfeed.github.io/dxfeed-graal-cxx-api/d6/d01/classdxfcpp_1_1_time_series_subscription_symbol.html)
  represents subscription to time-series events
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [IndexedEventSubscriptionSymbol](https://dxfeed.github.io/dxfeed-graal-cxx-api/df/d02/classdxfcpp_1_1_indexed_event_subscription_symbol.html)
  represents subscription to a specific source of indexed events
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [WildcardSymbol.ALL](https://dxfeed.github.io/dxfeed-graal-cxx-api/dd/d03/structdxfcpp_1_1_wildcard_symbol.html)  represents a
  *wildcard* subscription to all events of the specific event type
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [CandleSymbol](https://dxfeed.github.io/dxfeed-graal-cxx-api/da/d01/structdxfcpp_1_1_candle_symbol.html)  symbol used
  with [DXFeedSubscription](https://dxfeed.github.io/dxfeed-graal-cxx-api/d9/d03/classdxfcpp_1_1_d_x_feed_subscription.html) class to
  subscribe for [Candle](https://dxfeed.github.io/dxfeed-graal-cxx-api/de/d03/classdxfcpp_1_1_candle.html) events
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API

### Subscriptions & Models

- [DXFeedSubscription](https://dxfeed.github.io/dxfeed-graal-cxx-api/d9/d03/classdxfcpp_1_1_d_x_feed_subscription.html) is a subscription for
  a set of symbols and event types
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API

### IPF & Schedule

- [InstrumentProfile](https://dxfeed.github.io/dxfeed-graal-cxx-api/d8/d01/classdxfcpp_1_1_instrument_profile.html) represents basic
  profile information about a market instrument
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API ([DxFeedIpfConnect](https://github.com/dxFeed/dxfeed-graal-cxx-api/tree/main/samples/cpp/DxFeedIpfConnect))
- [InstrumentProfileReader](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/ipf/InstrumentProfileReader.html) reads
  instrument profiles from the stream using Instrument Profile Format (IPF).
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [InstrumentProfileConnection](https://dxfeed.github.io/dxfeed-graal-cxx-api/db/d00/classdxfcpp_1_1_instrument_profile_connection.html)
  connects to an instrument profile URL and reads instrument profiles with support of streaming live updates.
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API
- [InstrumentProfileCollector](https://dxfeed.github.io/dxfeed-graal-cxx-api/de/d03/classdxfcpp_1_1_instrument_profile_collector.html)
  collects instrument profile updates and provides the live instrument profiles list
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API ([DxFeedLiveIpfSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedLiveIpfSample/src/main.cpp))
- [Schedule](https://dxfeed.github.io/dxfeed-graal-cxx-api/d1/d00/structdxfcpp_1_1_schedule.html) provides API to retrieve and
  explore various exchanges’ trading schedules and different financial instrument
  classes
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API ([ScheduleSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/ScheduleSample/src/main.cpp))

### Services

- [OnDemandService](https://dxfeed.github.io/dxfeed-graal-cxx-api/d4/d02/structdxfcpp_1_1_on_demand_service.html) provides on-demand
  historical tick data replay
  controls ([OnDemandSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/OnDemandSample/src/main.cpp))
  - [ ] dxFeed Graal C API
  - [x] dxFeed Graal C++ API

<!-- 2024-04-15 18:30 -->