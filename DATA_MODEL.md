# Data model: why a value is missing or looks strange

This page explains how the events of the dxFeed model are delivered and what some fields mean, so that an expected
behavior is not mistaken for a defect of the API. For connection problems see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## Contents

- [Delivery contracts](#delivery-contracts)
- [Trade and TradeETH](#trade-and-tradeeth)
- [Event flags and snapshots](#event-flags-and-snapshots)
- [Missing values and NaN](#missing-values-and-nan)
- [Time and sequence](#time-and-sequence)
- [Order sources and scope](#order-sources-and-scope)
- [Crossed and locked books](#crossed-and-locked-books)
- [Candle symbols](#candle-symbols)
- [Wildcard subscription](#wildcard-subscription)
- [See also](#see-also)

## Delivery contracts

| Events                                                                       | Contract                                           |
|------------------------------------------------------------------------------|----------------------------------------------------|
| `Quote`, `Trade`, `TradeETH`, `Summary`, `Profile`, `OrderImbalance`         | ticker (conflated): the latest value per symbol    |
| `TimeAndSale`                                                                | stream: the events are not conflated               |
| `TimeAndSale` in a time-series subscription                                  | history event                                      |
| `Order`, `AnalyticOrder`, `SpreadOrder`, `OtcMarketsOrder`, ...              | history (indexed events)                           |

For a ticker event only the latest value of a symbol is kept while a consumer is slow. The last value is delivered on
a new subscription, but intermediate values are discarded as stale, so a `Quote` listener does not see every change
of the market. This is the design, not a loss of data.

Practical consequence: to get every trade, subscribe to `TimeAndSale`, not to `Trade`. `Trade` is a snapshot of the last
trade and its day volume; trades between two updates can be skipped. Stream events are not conflated, but a listener that
is too slow can still lose them: `DXFeedSubscription::getTotalDroppedEvents()` counts such losses.

Orders are indexed events: they are also conflated per symbol and index, so an order book must be built from the events
with their flags, see below.

## Trade and TradeETH

- `Trade` is the last trade of the regular trading hours (RTH) with the volume and turnover of the whole trading day.
  During the extended hours the `Trade` event updates only the day volume and the day turnover.
- `TradeETH` is the last trade of the extended trading hours (pre market and post market) with the volume and turnover
  of the extended hours only. It is not defined for symbols that have no concept of extended hours.
- On the first trade of the regular hours `TradeETH` is generated once with `isExtendedTradingHours()` equal to
  `false`, and during the regular hours it is not updated afterward.

## Event flags and snapshots

Indexed events (orders, time series) carry flags, see [EventFlag.hpp](include/dxfeed_graal_cpp_api/event/EventFlag.hpp):

| Flag             | Meaning                                                                                 |
|------------------|-----------------------------------------------------------------------------------------|
| `TX_PENDING`     | the event is a part of a transaction that is not complete yet                           |
| `REMOVE_EVENT`   | remove the event with this index                                                        |
| `SNAPSHOT_BEGIN` | the loading of a snapshot starts (on a new subscription)                                |
| `SNAPSHOT_END`   | the source has sent all the data for the subscription                                   |
| `SNAPSHOT_SNIP`  | a limit on the amount of data is reached: more data may exist but will not be provided  |

Processing the flags by hand is error-prone: the state of every source is tracked separately, and a book is consistent
only after the whole list of events of one call. Use a model that does it for you:
[IndexedTxModel](include/dxfeed_graal_cpp_api/model/IndexedTxModel.hpp),
[IndexedEventTxModel](include/dxfeed_graal_cpp_api/model/IndexedEventTxModel.hpp) (one symbol and one source; delivers
complete snapshots and transactions only),
[TimeSeriesTxModel](include/dxfeed_graal_cpp_api/model/TimeSeriesTxModel.hpp) and
[MarketDepthModel](include/dxfeed_graal_cpp_api/model/MarketDepthModel.hpp) (an order book of the given depth).

## Missing values and NaN

A `double` field that has not been received is initialized as `NaN` in a fresh event (for example the bid and ask price
and size of `Quote`, the price and size of a trade), so test such a value with `std::isnan`.

Do not read more into it than this: the documentation does not state that `NaN` means "no value" for every field. For
orders there is an explicit rule: a size of `0` or `NaN` is a signal to remove a previously received order
(`OrderBase::hasSize()` is `false`), see [OrderBase.hpp](include/dxfeed_graal_cpp_api/event/market/OrderBase.hpp).

## Time and sequence

- `getTime()` of an event is in milliseconds since the UNIX epoch.
- `Quote::getBidTime()` and `Quote::getAskTime()` are transmitted with second precision.
- Nanosecond precision is only available when the endpoint is created with the system property
  `dxscheme.nanoTime=true` (`DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY`). Then `getTimeNanos()` (nanoseconds since the
  epoch) and `getTimeNanoPart()` (the part of the time below a millisecond) are meaningful. Without the property they are
  not guaranteed to carry a value.
- `getSequence()` distinguishes events with the same time. It does not have to be unique or sequential, so use it to
  order events within one millisecond, not as an identifier. The index of a `TimeAndSale` is composed of its time and
  sequence.

## Order sources and scope

Subscribing to `Order` with a plain symbol (without a source) means all the supported sources, not only
`OrderSource::DEFAULT`. The synthetic and aggregate sources then come as separate `_BID` and `_ASK` sources
(`OrderSource::AGGREGATE_BID`, `OrderSource::AGGREGATE_ASK`, and the like). The system property
`dxscheme.unitaryOrderSource=true` selects the unitary sources (for example `OrderSource::AGGREGATE`) instead. Which of
the sources send data for a symbol depends on the data provider.

To subscribe to one source, wrap the symbol into
[IndexedEventSubscriptionSymbol](include/dxfeed_graal_cpp_api/api/osub/IndexedEventSubscriptionSymbol.hpp):

```cpp
auto sub = dxfcpp::DXFeedSubscription::create(dxfcpp::Order::TYPE);

sub->addSymbols(dxfcpp::IndexedEventSubscriptionSymbol("AAPL", dxfcpp::OrderSource::NTV));
```

An order book of one source with the model:

```cpp
auto model = dxfcpp::MarketDepthModel<dxfcpp::Order>::newBuilder()
                 ->withFeed(dxfcpp::DXEndpoint::getInstance()->getFeed())
                 ->withSources({dxfcpp::OrderSource::NTV})
                 ->withSymbol("AAPL")
                 ->withDepthLimit(10)
                 ->withListener([](const std::vector<std::shared_ptr<dxfcpp::Order>> &buy,
                                   const std::vector<std::shared_ptr<dxfcpp::Order>> &sell) { /* ... */ })
                 ->build();
```

[Scope](include/dxfeed_graal_cpp_api/event/market/Scope.hpp) tells what an order of a book represents:

| Scope              | Meaning                                                                 |
|--------------------|-------------------------------------------------------------------------|
| `Scope::COMPOSITE` | best bid and offer for the whole market                                 |
| `Scope::REGIONAL`  | best bid and offer for an exchange code                                 |
| `Scope::AGGREGATE` | aggregate for a price level, or best bid and offer for a market maker   |
| `Scope::ORDER`     | an individual order                                                     |

The sources are described in [OrderSource.hpp](include/dxfeed_graal_cpp_api/event/market/OrderSource.hpp).

## Crossed and locked books

A book built from the data can be crossed (best bid above best ask) or locked (equal prices) for a short time. The
reasons are listed in the class description of `MarketDepthModel`: lost or conflated data (a removal that never
arrived), a slow client, and sources that deliver one side only or a book that combines several sources or exchanges,
whose prices can legitimately cross for a moment. The model shows the orders as they are and does not correct them.

`MarketDepthModel::Builder::withCrossedBookWarning(enabled, period)` makes the model report a crossed book as a warning
through `ErrorManager`. It is off by default, and the default period between two warnings is 1 minute:

```cpp
->withCrossedBookWarning(true, std::chrono::minutes{1})
```

The warning is reported only when the best bid price is strictly above the best ask price. A locked book is not
reported, and a `NaN` price is not a crossing.

## Candle symbols

A wrong candle symbol is valid for the parser and for the feed: it gives no error, only no candles or other candles
than expected. The page [Candle symbols](include/dxfeed_graal_cpp_api/event/candle/CandleSymbol.hpp) describes the
format and the typical mistakes. `CandleSymbol::valueOf()` of a string and the `_c` literal report a warning for the
likely ones, for example:

- no period gives tick candles: write `AAPL{=d}` or `AAPL{=1m}`, and `AAPL{=t}` for ticks on purpose;
- the period type is matched by the beginning of its name: `M` is a minute (`AAPL{=5M}` is 5 minutes), a month is
  `mo`; `PR` is a price period, a Renko one is `pr`.

## Wildcard subscription

The wildcard symbol `WildcardSymbol::ALL` (`*`) works only if the endpoint is created with the system property
`dxfeed.wildcard.enable=true` (`DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY`). It can create a very high network and CPU
load and is meant for low-frequency events, see
[WildcardSymbol.hpp](include/dxfeed_graal_cpp_api/api/osub/WildcardSymbol.hpp). Any other symbol that starts with `*` is
ignored.

## See also

- [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- [README.md#documentation](README.md#documentation): the dxFeed Knowledge Base (delivery contracts, market events,
  order sources, order book reconstruction, symbology)
- [Quote.hpp](include/dxfeed_graal_cpp_api/event/market/Quote.hpp),
  [Trade.hpp](include/dxfeed_graal_cpp_api/event/market/Trade.hpp),
  [TradeETH.hpp](include/dxfeed_graal_cpp_api/event/market/TradeETH.hpp),
  [TimeAndSale.hpp](include/dxfeed_graal_cpp_api/event/market/TimeAndSale.hpp),
  [Order.hpp](include/dxfeed_graal_cpp_api/event/market/Order.hpp),
  [LastingEvent.hpp](include/dxfeed_graal_cpp_api/event/LastingEvent.hpp)
