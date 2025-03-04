* **\[MDAPI-30]\[C++]** Added LastEventsConsole sample
* **\[MDAPI-27]\[C++]** Implemented OptionChain
    * Added `OptionChain` class. 
    * Added `OptionSeries` class. 
    * Added `OptionChainsBuilder` class.
    * Added `OptionChainSample` example.
* **\[MDAPI-77]\[C++]** Added support for retrieving indexed events from feed
* **\[MDAPI-78]\[C++]** Added support for retrieving time-series events from feed
* **\[MDAPI-36]\[C++]** Implemented DXFeedTimeSeriesSubscription
    * Added `DXFeedTimeSeriesSubscription` class.
    * Added `DXFeed::createTimeSeriesSubscription` methods.
* **\[MDAPI-214]\[C++]\[Console]** Added MarketDepthModelSample
    * Added `MarketDepthModelSample`.
    * Added `CmdArgsUtils::parseEventSources` method.
* **\[MDAPI-216]\[C++]\[Console]** Added PriceLevelBook sample
* **\[MDAPI-76]\[C++]** Implemented TextMessage event
* Migrated to Graal SDK v2.2.1
* **\[MDAPI-214]\[C++]\[Console]** Added MarketDepthModelSample
    * Added `MarketDepthModelSample`.
    * Added `CmdArgsUtils::parseEventSources` method.

## v4.0.0

* **\[MDAPI-38]\[C++]** Added MultipleMarketDepthSample sample
    * Added `MultipleMarketDepthSample` sample.
    * Added `AuthSample` sample.
    * Added `ReconnectSample` sample.
    * Added `IncOrderSnapshotSample` sample.
* Migrated to Graal SDK v2.1.2
* **\[MDAPI-211]\[C++]** Implement logging management
    * Added `Logging` class.
* **\[MDAPI-82]\[C++]** Implement MarketDepthModel
    * Added `MarketDepthModel` class.
    * Added `MarketDepthModelListener` class.
* **\[MDAPI-84]\[C++]** Implement TimeSeriesTxModel
    * Added `TimeSeriesTxModel` class.
* **\[MDAPI-83]\[C++]** Implement IndexedTxModel
    * Added `TxModelListener` class.
    * Added `IndexedTxModel` class.
    * Added `EventSourceWrapper` class needed to pass heterogeneous event sources using a container.
* **\[MDAPI-79]\[C++]** Retrieve promise-based events from feed
    * Added `PromiseList<E>` class where `E` - event type: a list of event receiving results that will be completed
      normally or exceptionally in the future.
      It is a `std::vector<Promise<std::shared_ptr<E>>>` wrapper with Graal semantics.
    * Added `Promise<void>` class.
    * Added `Promises` class: utility methods to manipulate `Promise<>` promises.
        * Added `Promises::allOf(Collection &&collection)` method that returns a new promise that completes when all
          promises from the given collection complete normally or exceptionally.
    * Fixed the `Promise<std::shared_ptr<E>>` semantics.
    * Added `DXFeed::getLastEventPromise` method.
    * Added `DXFeed::getLastEventsPromises` method.
    * Added `DXFeed::getIndexedEventsPromise` method.
    * **\[BREAKING]** The `DXFeed::getTimeSeriesPromise` now returns
      `std::shared_ptr<Promise<std::vector<std::shared_ptr<E>>>>`
    * **\[BREAKING]** The `IndexedEventSubscriptionSymbol::getSource` method now returns
      `std::unique_ptr<IndexedEventSource>`

## v3.0.0

* **\[MDAPI-37]\[C++]** Retrieve latest events from feed.
    * Added `SymbolWrapper::toStringUnderlying` method that returns a string representation of the underlying symbol
      object.
    * Added `EventType::assign`. All `EventType` successors can populate their fields using `EventType` successors of
      the same type.
    * Added `DXFeed::getLastEventIfSubscribed` method.
    * Added `DXFeed::getLastEvent` and `DXFeed::getLastEvents` methods that use the `DXFeed::getLastEventIfSubscribed`
      method.
* **\[MDAPI-113]\[C++]\[Tools]** Tools should report invalid event type.
    * Added classes: `RuntimeException`, `InvalidArgumentException`.
    * `InvalidArgumentException`, `GraalException`, `JavaException` are now descendants of the `RuntimeException` class,
      which can collect stacktrace.
    * Now an `InvalidArgumentException` exception is thrown instead of the `std::invalid_argument` exception.
    * `Tools` now reports incorrect event types specified by the user.
* **\[MDAPI-80]\[C++]\[IPF]** Implement custom fields in InstrumentProfile.
    * The API was migrated to Graal SDK v1.1.22.
    * Added methods:
        * `InstrumentProfile::getField`
        * `InstrumentProfile::setField`
        * `InstrumentProfile::getNumericField`
        * `InstrumentProfile::setNumericField`
        * `InstrumentProfile::getDateField`
        * `InstrumentProfile::setDateField`
        * `InstrumentProfile::getNonEmptyCustomFieldNames`
    * **\[BREAKING]**: All `toString` methods can now throw exceptions.
* **\[MDAPI-26]\[C++]** Migrate to Graal SDK v1.1.21.
    * Added `Day::getSessions` method.
    * New order sources added: `CEDX` and `cedx`.
    * Added the ability to use dxFeed Graal SDK stub. In other words, now API can be built for any 32 and 64 platforms.
    * Added `TimePeriod` class.
    * Added `DXFeedSubscription::getAggregationPeriod` and `DXFeedSubscription::setAggregationPeriod` methods.
    * Added `DXFeedSubscription::getEventsBatchLimit` and `DXFeedSubscription::setEventsBatchLimit` methods.
    * Added `AuthToken` class.
    * Added `InstrumentProfileReader::readFromFile(address, token)` method.

## v2.0.0