# Troubleshooting

This page helps to find the cause when the C++ API receives no data or the connection drops, and tells what to check
first. The reference tables of the connection errors are in the headers, see [See also](#see-also).

- [First steps](#first-steps)
- [Connection errors](#connection-errors)
- [No data](#no-data)
- [Latency](#latency)
- [Reconnect contract](#reconnect-contract)
- [See also](#see-also)

## First steps

1. Run the `Diagnose` command of the [Tools](README.md#tools). It checks the way to the data step by step and prints a
   verdict (`DATA`, `PARTIAL DATA`, `CONNECTED, NO DATA`, `CANNOT CONNECT`, `CANNOT RESOLVE`):

   ```shell
   Tools Diagnose demo.dxfeed.com:7300 Quote,Trade AAPL,IBM --report diagnose.txt
   ```

   The verdicts are described in [Versions and addresses in the log](README.md#versions-and-addresses-in-the-log).
2. Find the version lines in the log of the program: the `[dxfcpp] INFO` line with the versions of the C++ API and of
   the Graal Native SDK, and `Using QDS-<version>, (C) Devexperts` with the version of QD inside the SDK. The same
   section of the README explains them.
3. Write the log of the SDK (QD) to a file, before the first endpoint is created:

   ```cpp
   Logging::init("qd.log", Logging::Level::INFO);
   ```

   There is also an overload with a second file for the errors and warnings only:
   `Logging::init(logFile, errFile, logLevel, errFileLevel)`. The connection lines quoted below are in this log.
4. When you ask the support, attach the file written by `--report`: logins, passwords and tokens are hidden in it. Add
   the log file of the SDK and the versions from step 2.

## Connection errors

The SDK writes a failed connect or a dropped connection to its log, and the C++ API keeps the newest one as a
`ConnectionError` with a `Category`. The log line, the category and the first check:

| Log line or exception | Category (`ConnectionError::Category`) | Likely cause | First check |
|-----------------------|----------------------------------------|--------------|-------------|
| `Failed to connect to 127.0.0.1:64064`, then `java.net.ConnectException: Connection refused: connect` (Windows; on Linux `Connection refused`) | `CONNECT_FAILED` | Nothing listens on the port; a wrong port or host; a firewall; the server is down | The host and the port in the address; `Tools Diagnose` prints the result of a TCP connect to each IP |
| `Failed to resolve IPs for diagnose-test.invalid`, then `Failed to connect to diagnose-test.invalid:7300` with `java.net.UnknownHostException: diagnose-test.invalid` | `DNS` | The host name does not resolve: a typo, no DNS server, a proxy | The spelling of the host name; resolve it with the system tools (`nslookup`, `ping`) |
| `Disconnected from 127.0.0.1:64088 because of Connection closed by remote side` (`java.io.EOFException`) | `CLOSED_BY_REMOTE` | The server closed the connection: it restarted; it rejected the login or the entitlement; a wrong port or protocol; a proxy in the way. The client cannot tell which | The login and the address options; the port and the protocol of the server; the server side logs |
| `java.net.SocketException: Connection reset` | `IO` | The peer or a middlebox reset the connection | The network path: a firewall, a proxy, a load balancer |
| `heartbeat timeout exceeded: disconnecting` (no exception) | `HEARTBEAT_TIMEOUT` | The remote side sent no data and no heartbeat for 2 minutes by default: a stalled network or server | The network path; the heartbeat timeout can be changed by the address option `[heartbeatTimeout=...]` |
| `java.net.SocketTimeoutException` | `READ_TIMEOUT` | Nothing arrived for the socket read timeout (5 minutes by default). The 2 minutes of the heartbeat timeout are shorter, so on the default settings it usually fires first (not guaranteed) | Same as for the heartbeat timeout |
| `javax.net.ssl.SSLException` and other handshake errors | `TLS` | A `tls+` address of a port without TLS, or the reverse; a certificate that is not trusted | The TLS flag in the address and the port; the certificate of the server |
| `java.util.concurrent.CancellationException` | none: not a connection error | A promise was not completed in time (`await` with a timeout), was cancelled, its thread was interrupted, or the symbol is rejected by the filter of the feed | The promise, not the network: is the endpoint connected, is the symbol allowed |

The exact texts of the JDK can differ between the JDK versions and the systems, search by the exception class. Any
other error has the category `OTHER`: read its message.

A malformed address is not a connection error: `DXEndpoint::connect()` throws at once and there is no reconnect loop.

To read the error in code, use `DXEndpoint::getLastConnectionError()`, which returns a `std::optional<ConnectionError>`,
or add a listener with `DXEndpoint::addMessageConnectorListener()` and read `MessageConnectorInfo::lastError`.
`DXEndpoint::getConnectors()` gives the state, the current addresses and the last error of each connector. Failed
connect attempts do not change the state of the endpoint: it stays `CONNECTING` between them, so a state listener
(`DXEndpoint::addStateChangeListener()`) does not see them; read the error in the connector listener. A successful
connection clears no error: compare the time or the count of the error with those seen before. The full category table
is in [MessageConnectorInfo.hpp](include/dxfeed_graal_cpp_api/api/MessageConnectorInfo.hpp) (the page "Connection
troubleshooting"). These functions are experimental and may change.

## No data

Connected, but no events? Go through the causes from the most frequent:

| Cause | First check |
|-------|-------------|
| Not entitled: depending on the server, it accepts the connection and sends nothing (the state stays `CONNECTED`, no error), or it closes the connection (`CLOSED_BY_REMOTE`) | `Tools Diagnose` prints `CONNECTED, NO DATA`; check the login of the address and the entitlements with the data provider |
| A wrong endpoint role | `DXEndpoint::Role::FEED` is the default. `ON_DEMAND_FEED` is not connected to a data provider until `OnDemandService::replay()`; `LOCAL_HUB` has no network connections; `PUBLISHER` and `STREAM_PUBLISHER` publish events; in `STREAM_FEED` `DXFeed::getLastEvent()` does not work and the promises of the feed complete exceptionally |
| The symbol is not in the endpoint filter or the IPF of the provider | Try a symbol that is known to exist, for example `AAPL` on `demo.dxfeed.com:7300` |
| A wrong symbol form | See the list below |
| A wildcard `*` subscription | It needs the endpoint property `dxfeed.wildcard.enable=true` (`DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY`) and a permission of the provider |
| The symbol does not trade now | A symbol outside of its trading session is silent. Check another symbol that is active |
| The data stops while the connection stays up | Turn on the silence watchdog of the subscription: `DXFeedSubscription::setSilenceWatchdog(timeout)`. It is off by default and reports a warning through `ErrorManager` when no events arrive for the timeout while the endpoint is `CONNECTED` |

Symbol forms to check:

- Candle symbols: the form is `AAPL{=d}`, `AAPL{=1m}`; a string without a period gives tick candles, and `M` or `PR`
  are read as the minute and the price, not as the month (`mo`) and the Renko price (`pr`). `CandleSymbol::valueOf()`
  reports a warning about such strings, see the page "Candle symbols" in
  [CandleSymbol.hpp](include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp).
- Order sources: a subscription to an indexed event with a plain symbol (`AAPL`) is a subscription to all supported
  sources. The aggregate sources come as the `_BID` and `_ASK` variants (for example `AGGREGATE_BID`), unless the
  property `dxscheme.unitaryOrderSource=true` is set. To get one source, use `IndexedEventSubscriptionSymbol` with an
  `OrderSource`.
- Regional symbols have an exchange code after `&`, for example `AAPL&Q`.

Warnings of the library (a suspicious candle symbol, a silent subscription) go to `ErrorManager`, which writes them to
`stderr` by default. If you do not see them, check that its handler is not replaced or turned off.

## Latency

- Measure the delay as the receive time minus the time of the event: this is what `Tools Diagnose` prints. The time of
  the event is `getTime()` of `Quote`, `Trade`, `TimeAndSale` and the like, in milliseconds since the UNIX epoch (UTC).
  `getEventTime()` is not this time: it is zero for the events that come from a network connection.
- The `bidTime` and the `askTime` of `Quote` have a precision of a second. A precision of nanoseconds (`getTimeNanos()`)
  needs the endpoint property `dxscheme.nanoTime=true` (`DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY`).
- The clocks of this computer and of the data source differ: the measured delay includes the difference. Synchronize
  the time and compare with a known good host.
- A slow listener delays all the events of its subscription: the lasting events (`Quote`, ...) are conflated and arrive
  late, the other events may be dropped (`DXFeedSubscription::getTotalDroppedEvents()`). The library reports a warning
  through `ErrorManager` when the listeners of a subscription are slow (the system property
  `dxfcpp.subscription.slowListenerThreshold`, `0.1s` by default) and, when you turn it on, when the events lag behind
  (`dxfcpp.subscription.eventLagThreshold`, off by default; set it to several seconds). The period of the repeated
  warnings is `dxfcpp.subscription.warningPeriod`. See the page "Listeners and threads" in
  [DXFeedSubscription.hpp](include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp).

## Reconnect contract

What the API does after a lost connection, and what it does not promise.

**Reconnect and the subscriptions.**

- The reconnect is automatic: a client connection (`host:port`) is re-created after a failed attempt or a drop. The
  client sends the subscriptions again on each new connection, nothing has to be subscribed anew. What the server sends
  after that (a snapshot, the events missed during the outage) is not guaranteed.

**States.**

- The states are `NOT_CONNECTED`, `CONNECTING`, `CONNECTED` and `CLOSED`. After a network loss the endpoint goes
  `CONNECTED` -> `CONNECTING` -> `CONNECTED`; there is no state event per failed attempt.
- `NOT_CONNECTED` comes only after `disconnect()`, at the end of a file source, or for a new endpoint that was not
  connected yet; a network loss does not lead to it.
- With several connectors the endpoint is `CONNECTED` if any of them is connected.

**The delay before an attempt.**

- The delay is 10 seconds by default. To change it, use the address option in milliseconds:
  `demo.dxfeed.com:7300[reconnectDelay=5000]`.
- The wait is random, from 1 to 2 times the remaining delay. It does not grow with the number of failures: there is no
  exponential backoff. If the previous connection lived at least the delay, the next attempt is immediate.

**Several addresses.**

- `host1:port1,host2:port2` is one connection that moves to the next address of the list after a failure. By default the list is
  shuffled (`connectOrder=shuffle`), so no host is the first one. The address option `[connectOrder=ordered]` (also
  `random`, `priority`) changes it. All the IP addresses of a DNS name are tried, and the name is resolved again when
  the list is exhausted.
- `(addr1)(addr2)` is two independent connections at the same time, not a failover.

**Calls.**

- `connect()` with the same address does nothing while connected. After `disconnect()` the same address connects again.
- `disconnect()` keeps the subscriptions and the last values; `disconnectAndClear()` also clears the data; `close()` is
  final.

**Not restored and not guaranteed.**

- The last values that are cached are not marked as stale after a drop: they stay until new data arrives. Use the
  state of the endpoint or the connectors to know that the connection is lost.
- The events missed during the outage are not guaranteed to be replayed.
- A time-series subscription sends its `fromTime` again, so the history is requested again; what the server returns and
  how it merges with the local history is not guaranteed.
- A fresh snapshot (for example of an order book) after a reconnect depends on the server and is not guaranteed.

## See also

- [Connection troubleshooting](include/dxfeed_graal_cpp_api/api/MessageConnectorInfo.hpp): the categories of the
  connection errors and the Java exceptions
- [Error handling](include/dxfeed_graal_cpp_api/exceptions/ErrorManager.hpp): `ErrorManager`, the warnings and the
  errors without a caller
- [Listeners and threads](include/dxfeed_graal_cpp_api/api/DXFeedSubscription.hpp): slow listeners, the lag of events,
  the silence watchdog
- [Candle symbols](include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp): the form of a candle symbol and the
  warnings
- [Isolate arguments](include/dxfeed_graal_cpp_api/system/System.hpp): system properties that must be set before the
  first Java code runs
- [Data model](DATA_MODEL.md): events, conflation, snapshots, order sources
- [README](README.md): [Tools](README.md#tools) and [Versions and addresses in the log](README.md#versions-and-addresses-in-the-log)
