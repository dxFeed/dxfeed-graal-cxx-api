<picture>
 <source media="(prefers-color-scheme: dark)" srcset="docs/images/logo_dark.svg">
 <img alt="light" src="docs/images/logo_light.svg">
</picture>

This package provides access to [dxFeed market data](https://dxfeed.com/market-data/).
The library is built as a language-specific wrapper over
the [dxFeed Graal Native](https://dxfeed.jfrog.io/artifactory/maven-open/com/dxfeed/graal-native-api/) library,
which was compiled with [GraalVM Native Image](https://www.graalvm.org/latest/reference-manual/native-image/)
and [dxFeed Java API](https://docs.dxfeed.com/dxfeed/api/overview-summary.html) (our flagman API).

:information_source: If you already use [dxFeed C  API](https://github.com/dxFeed/dxfeed-c-api), please see
the [Overview](#overview) section.<br>
:warning: It’s an **alpha** version and still under active development. **Don’t use it in a production environment.**

[![Build](https://github.com/dxFeed/dxfeed-graal-cxx-api/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/dxFeed/dxfeed-graal-cxx-api/actions/workflows/build.yml)
![](https://img.shields.io/badge/C++%20standard-C++20-blueviolet) ![](https://img.shields.io/badge/C%20standard-C11-blueviolet)
![Platform](https://img.shields.io/badge/platform-win--x64%20%7C%20linux--x64%20%7C%20osx--x64-lightgrey)
[![License](https://img.shields.io/badge/license-MPL--2.0-orange)](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/master/LICENSE)

## Table of Contents

- [Overview](#overview)
    * [Reasons for the New C/C++ API Repository](#reasons-for-the-new-net-api-repository)
    * [Benefits of the New Version](#benefits-of-the-new-version)
    * [Milestones](#milestones)
    * [Future Development](#future-development)
    * [Migration](#migration)
    * [Implementation Details](#implementation-details)
    * [Architectural Restrictions and Other Limitations in the Old Version](#architectural-restrictions-and-other-limitations-of-the-old-version)
- [Documentation](#documentation)
- [Installation](#installation)
- [Usage](#usage)
- [Samples](#samples)
- [Current State](#current-state)
- [Dependencies](DEPENDENCIES.md)
- [How To Build](HOW_TO_BUILD.md)
- [3rd Party Licenses](THIRD_PARTY_LICENSES.md)

## Overview

### Reasons for the New C/C++ API Repository

The old version of [dxFeed C API](https://github.com/dxFeed/dxfeed-c-api), has several [architectural restrictions](#architectural-restrictions-and-other-limitations-of-the-old-version)
that prevent us from providing a state-of-the-art technological solution.

### Benefits of the New Version

- :rocket: Increased performance
- :milky_way: Wider functionality
- :gemini: Identical programming interfaces to our best API
- :thumbsup: Higher quality of support and service

### Milestones

Feature development has already stopped for the [old version](https://github.com/dxFeed/dxfeed-c-api) of dxFeed C
API.

We expect the new repository to go into production in Q4’2023.
At the same time, the old version will be considered deprecated, and at the end of 2024, we plan to end the service.
If you’re already our customer and have difficulty with a future transition, please contact us via
our [customer portal](https://jira.in.devexperts.com/servicedesk/customer/portal/1/create/122).

### Future Development

Features planned with **high priority**:

* Add unit tests and conduct different types of testing
* Add necessary entities for more convenient API
  usage ([IPF](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/ipf/InstrumentProfile.html), [TimeSeriesEventModel](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/model/TimeSeriesEventModel.html), [OrderBookModel](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/model/market/OrderBookModel.html), [GetTimeSeriesPromise](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getTimeSeriesPromise-java.lang.Class-java.lang.Object-long-long-),
  etc.)
* Provide more samples
* Provide performance test results along with a comparison with the old API version

---
Features planned for the **next stage**:

* Implement a model
  of [incremental updates](https://kb.dxfeed.com/en/data-services/real-time-data-services/-net-api-incremental-updates.html)
  in Java API and add it to С/C++ API
* Implement OrderBookModel with advanced logic (e.g., OnNewBook, OnBookUpdate, OnBookIncrementalChange) in Java API and
  add it to C/C++ API
* Add samples or implement a convenient API
  for [Candlewebservice](https://kb.dxfeed.com/en/data-services/aggregated-data-services/candlewebservice.html)

### Migration

To help you rewrite the existing API calls, we’ve prepared [samples](#samples) demonstrating how to work with the new
API and how several functionalities are implemented. More examples will follow. The table below shows the sample mapping
between the old and new versions.

Our support team on our [customer portal](https://jira.in.devexperts.com/servicedesk/customer/portal/1/create/122) is
ready to answer any questions and help with the transition.

#### Sample Mapping

|  #  | Sample                                                                                                                            | Old Version                                                                                                                           | New Version                                              |
|:---:|:----------------------------------------------------------------------------------------------------------------------------------|:--------------------------------------------------------------------------------------------------------------------------------------|:---------------------------------------------------------|
|  1  | How to subscribe to `Quote`, `Trade`, `TradeETH`, `Order`, `SpreadOrder`, `AnalyticOrder`, `TimeAndSale` events                   | [CommandLineSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/CommandLineSample)                                     | <!--[-->EventSample<!--](samples/cpp/EventsSample)-->    |
|  2  | How to subscribe to `Candle` event                                                                                                | [CandleSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/CandleSample)                                               | *Q2’2023*, please see [TBD](#future-development) section |
|  3  | How to subscribe to `Order`, `SpreadOrder`, `Candle`, `TimeAndSale`, `Greeks`, `Series` snapshots                                 | [SnapshotConsoleSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/SnapshotConsoleSample)                             | *Q2’2023*, please see [TBD](#future-development) section |
|  4  | How to subscribe to depth of market                                                                                               | [PriceLevelBookSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/PriceLevelBookSample)                               | *Q2’2023*, please see [TBD](#future-development) section |
|  5  | How to subscribe to order snapshot with incremental updates                                                                       | [IncSnapshotConsoleSample](https://github.com/dxFeed/dxfeed-c-api/tree/master/samples/IncSnapshotConsoleSample)                       | *Q2’2023*, please see [TBD](#future-development) section |

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
will be amended, *"best practices"*, specific language restrictions)

Below is a scheme of this process:

<picture>
 <source media="(prefers-color-scheme: dark)" srcset="docs/images/scheme_dark.svg">
 <img alt="light" src="docs/images/scheme_light.svg">
</picture>

### Architectural Restrictions and Other Limitations of the Old Version

|  #  | Limitation                                                                                                                                                                                                                                                                                                                                    | How It’s Solved in the New Version                                                                                                                                                                                                                                                               |
|:---:|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|  1  | Single-threaded architecture limiting throughput.                                                                                                                                                                                                                                                                                             | Based on the Java API, each subscription object ([DXFeedSubscription](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeedSubscription.html)) *can* run on its own thread.                                                                                                                   |
|  2  | User code in event callbacks (for example, [dxf_event_listener_t](https://docs.dxfeed.com/c-api/group__event-data-structures-event-subscription-stuff.html#gac8bcb70cd4c8857f286f4be65e9522c6)) is executed in the socket read thread, which can significantly reduce throughput.                                                                           | Socket processing threads and callback threads are separated.                                                                                                                                                                                                                                    |
|  3  | In event callbacks, one market event type and one data portion always arrive (excluding snapshot subscription), which increases the load on the CPU with a large amount of incoming data.                                                                                                                                                     | Event callbacks can receive different market event types, and more than one by batch.                                                                                                                                                                                                            |
|  4  | It’s impossible to subscribe to data without getting [regionals](https://kb.dxfeed.com/en/data-model/exchange-codes.html) (if it is available for the market event) or only for a certain regional.                                                                                                                                           | ```subscription->addSymbols({"AAPL"});``` - [composite](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#quote-47603)<br>```subscription->addSymbols({"AAPL&Q"});``` - [regional](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#quote-x--regional-quote-). |
|  5  | It’s impossible to subscribe to Order event (excluding snapshot subscription) without getting: all [sources](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#order-x), Order by Quote (including regionals), Order by [MarketMaker](https://kb.dxfeed.com/en/data-model/qd-model-of-market-events.html#marketmaker-47603). | ```subscription->addSymbols(IndexedEventSubscriptionSymbol::create("AAPL", OrderSource::NTV));``` - [OrderSource](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/include/dxfeed_graal_native_cpp_api/event/market/OrderSource.hpp) determines which data is being subscribed to.                                                                                                                               |
|  6  | Data is mixed up when creating two subscriptions (regular and time series) for the same market event type. Both regular and time series data go to both subscriptions.                                                                                                                                                                        | Each subscription instance receives only the data requested.                                                                                                                                                                                                                                     |
|  7  | Each subsequent request for the same symbol set in a subscription instance overwrites the existing one in another subscription instance.                                                                                                                                                                                                      | Subscription instances and the data they receive are independent of each other.                                                                                                                                                                                                                  |
|  8  | Removing a symbol from one subscription instance caused it to be removed from all others.                                                                                                                                                                                                                                                     | Subscription instances and the data they receive are independent of each other.                                                                                                                                                                                                                  |
|  9  | Incorrect behavior when reading from a file (if a market event in the file hasn’t been subscribed to). Reading from a file always occurs at maximum speed. The supported format is binary only.                                                                                                                                               | ```endpoint->connect("file:tape.txt[format=text]");``` - processing a text file with at it's "real" speed by timestamps<br>```endpoint->connect("file:tape.bin[format=binary,speed=max]");``` - processing a binary file with max speed.                                                         |

## Documentation

Find useful information in our self-service dxFeed Knowledge Base or .NET API documentation:

- [dxFeed Graal C/C++ API documentation]()
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

## Installation

Download zip bundle, add to CMake project.

## Usage

```cpp
#include <iostream>
#include <dxfeed_graal_cpp_api/api.hpp>

int main() {
    using namespace dxfcpp;
    
    auto endpoint = DXEndpoint::newBuilder()
            ->withProperty("dxfeed.address", "demo.dxfeed.com:7300")
            ->build();
    
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

## Tools

<!-- 
[Tools](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/)
is a collection of tools that allow you to subscribe to various market events for the specified symbols. The tools can
be downloaded from [Release](https://github.com/dxFeed/dxfeed-graal-cxx-api/releases) (including self-contained versions)
* [Connect](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/Connect/ConnectTool.cpp)
  connects to the specified address(es) and subscribes to the specified events with the specified symbol
* [Dump](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/Dump/DumpTool.cpp)
  dumps all events received from address. This was designed to retrieve data from a file
* [PerfTest](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/tools/Tools/src/PerfTest/PerfTestTool.cpp)
  connects to the specified address(es) and calculates performance counters (events per second, memory usage, CPU usage,
  etc.)
-->

## Samples
<!--
* [ConvertTapeFile](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/ConvertTapeFile/src/main.cpp)
  demonstrates how to convert one tape file to another tape file with optional intermediate processing or filtering
* [DxFeedConnect](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedConnect/src/main.cpp)
  demonstrates how to subscribe various market events for the specified symbols
* [DxFeedFileParser](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedFileParser/src/main.cpp)
  is a simple demonstration of how events are read form a tape file
-->
* [DxFeedSample](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/DxFeedSample/src/main.cpp)
  is a simple demonstration of how to create multiple event listeners and subscribe to `Quote` and `Trade` events
* [PrintQuoteEvents](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/PrintQuoteEvents/src/main.cpp)
  is a simple demonstration of how to subscribe to the `Quote` event, using a `DxFeed` instance singleton
  and `dxfeed.properties` file
<!--
* [WriteTapeFile](https://github.com/dxFeed/dxfeed-graal-cxx-api/blob/main/samples/cpp/WriteTapeFile/src/main.cpp)
  is a simple demonstration of how to write events to a tape file
-->

## Current State

### Event Types

- [ ] [Order](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/Order.html) is a snapshot of the full available
  market depth for a symbol
- [ ] [SpreadOrder](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/SpreadOrder.html) is a snapshot of the
  full available market depth for all spreads
- [ ] [AnalyticOrder](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/AnalyticOrder.html) represents an
  extension of Order introducing analytic information, e.g., adding iceberg-related
  information to this order
- [x] [Trade](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/Trade.html) is a snapshot of the price and size
  of the last trade during regular trading hours and an overall day
  volume and day turnover
- [x] [TradeETH](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/TradeETH.html) is a snapshot of the price
  and size of the last trade during extended trading hours and the extended
  trading hours day volume and day turnover
- [ ] [Candle](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/candle/Candle.html) - event with open, high, low, and
  close prices and other information for a specific period
- [x] [Quote](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/Quote.html) is a snapshot of the best bid and
  ask prices and other fields that change with each quote
- [x] [Profile](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/Profile.html) is a snapshot that contains the
  security instrument description
- [x] [Summary](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/Summary.html) is a snapshot of the trading
  session, including session highs, lows, etc.
- [x] [TimeAndSale](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/market/TimeAndSale.html) - represents a trade or
  other market event with price, like market open/close price, etc.
- [x] [Greeks](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/option/Greeks.html) is a snapshot of the option
  price, Black-Scholes volatility, and Greeks
- [x] [Series](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/option/Series.html) is a snapshot of computed values
  available for all options series for a given underlying symbol based on options market prices
- [x] [TheoPrice](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/option/TheoPrice.html) is a snapshot of the
  theoretical option price computation that is periodically performed
  by [dxPrice](http://www.devexperts.com/en/products/price.html) model-free computation
- [x] [Underlying](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/option/Underlying.html) is a snapshot of computed
  values available for an option underlying symbol based on the market’s option prices
- [ ] [Configuration](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/misc/Configuration.html) is an event with an
  application-specific attachment
- [ ] [Message](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/misc/Message.html) is an event with an
  application-specific attachment

### Subscription Symbols

- [x] String
- [ ] [TimeSeriesSubscriptionSymbol](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/osub/TimeSeriesSubscriptionSymbol.html) -
  represents subscription to time-series events
- [ ] [IndexedSubscriptionSymbol](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/osub/IndexedEventSubscriptionSymbol.html) -
  represents subscription to a specific source of indexed events
- [x] [WildcardSymbol.ALL](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/osub/WildcardSymbol.html) - represents a
  *wildcard* subscription to all events of the specific event type
- [ ] [CandleSymbol](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/candle/CandleSymbol.html) - symbol used
  with [DXFeedSubscription](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeedSubscription.html) class to
  subscribe for [Candle](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/candle/Candle.html) events

### Subscriptions & Models

- [x] [CreateSubscription](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeedSubscription.html) creates a new
  subscription for multiple event types *attached* to a specified feed
- [ ] [CreateTimeSeriesSubscription](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeedTimeSeriesSubscription.html)
  extends DXFeedSubscription to conveniently subscribe to time series of events for a set of symbols and event
  types ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/api/DXFeedConnect.java))
- [ ] [GetLastEvent](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getLastEvent-E-) returns the last
  event for the specified event
  instance ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/api/DXFeedSample.java))
- [ ] [GetLastEvents](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getLastEvents-java.util.Collection-)
  returns the last events for the specified event instances list
- [ ] [GetLastEventPromise](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getLastEventPromise-java.lang.Class-java.lang.Object-)
  requests the last event for the specified event type and
  symbol ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/console/LastEventsConsole.java))
- [ ] [GetLastEventsPromises](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getLastEventsPromises-java.lang.Class-java.util.Collection-)
  requests the last events for the specified event type and symbol collection
- [ ] [GetLastEventIfSubscribed](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getLastEventIfSubscribed-java.lang.Class-java.lang.Object-)
  returns the last event for the specified event type and symbol if there’s a subscription for it
- [ ] [GetIndexedEventsPromise](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getIndexedEventsPromise-java.lang.Class-java.lang.Object-com.dxfeed.event.IndexedEventSource-)
  requests an indexed events list for the specified event type, symbol, and source
- [ ] [GetIndexedEventsIfSubscribed](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getIndexedEventsIfSubscribed-java.lang.Class-java.lang.Object-com.dxfeed.event.IndexedEventSource-)
  requests an indexed events list for the specified event type, symbol, and source if there’s a subscription for it
- [ ] [GetTimeSeriesPromise](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getTimeSeriesPromise-java.lang.Class-java.lang.Object-long-long-)
  requests time series of events for the specified event type, symbol, and time
  range ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/_simple_/FetchDailyCandles.java))
- [ ] [GetTimeSeriesIfSubscribed](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXFeed.html#getTimeSeriesIfSubscribed-java.lang.Class-java.lang.Object-long-long-)
  requests time series of events for the specified event type, symbol, and time range if there’s a subscription for it
- [ ] [TimeSeriesEventModel](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/model/TimeSeriesEventModel.html) - is a model
  of a list of time series
  events ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ui/swing/DXFeedCandleChart.java))
- [ ] [IndexedEventModel](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/model/IndexedEventModel.html) is a model of a
  list of indexed
  events ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ui/swing/DXFeedTimeAndSales.java))
- [ ] [OrderBookModel](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/model/market/OrderBookModel.html) is a model of
  convenient Order Book
  management ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ui/swing/DXFeedMarketDepth.java))

### IPF & Schedule

- [ ] [InstrumentProfile](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/ipf/InstrumentProfile.html) represents basic
  profile information about a market
  instrument ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ipf/DXFeedIpfConnect.java))
- [ ] [InstrumentProfileCollector](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/ipf/live/InstrumentProfileCollector.html)
  collects instrument profile updates and provides the live instrument profiles
  list ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ipf/DXFeedLiveIpfSample.java))
- [ ] [Schedule](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/schedule/Schedule.html) provides API to retrieve and
  explore various exchanges’ trading schedules and different financial instrument
  classes ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/schedule/ScheduleSample.java))

### Services

- [ ] [OnDemandService](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/ondemand/OnDemandService.html) provides on-demand
  historical tick data replay
  controls ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/ondemand/OnDemandSample.java))

### Endpoint Roles

- [x] [FEED](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#FEED) connects to the remote data
  feed provider and is optimized for real-time or delayed data processing (**this is a default role**)
- [x] [STREAM_FEED](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#STREAM_FEED) is similar to
  FEED and also connects to the remote data feed provider but is designed for bulk data parsing from files
- [ ] [LOCAL_HUB](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#LOCAL_HUB) is a local hub
  without the ability to establish network connections. Events published via publisher are delivered to local feed only.
- [ ] [PUBLISHER](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#PUBLISHER) connects to the
  remote publisher hub (also known as multiplexor) or creates a publisher on the local
  host ([Java API sample](https://github.com/devexperts/QD/blob/master/dxfeed-samples/src/main/java/com/dxfeed/sample/_simple_/WriteTapeFile.java))
- [ ] [STREAM_PUBLISHER](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#STREAM_PUBLISHER) is
  similar to PUBLISHER and also connects to the remote publisher hub, but is designed for bulk data publishing
- [ ] [ON_DEMAND_FEED](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.Role.html#ON_DEMAND_FEED) is similar
  to FEED, but it is designed to be used with OnDemandService for historical data replay only
