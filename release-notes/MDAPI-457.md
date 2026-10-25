* **\[MDAPI-457]\[C++]** Migrated to Graal SDK v3.11.0.
* **\[MDAPI-457]\[C++]** Added the requested aggregation period of an endpoint and the aggregation period info, as in the
  Java API.
    * `DXEndpoint::setRequestedAggregationPeriod()` sets the period that the endpoint requests from the server
      (`TimePeriod`, `std::chrono::milliseconds` or milliseconds); `std::nullopt` clears it. Zero requests real-time
      delivery; a period over 24 hours throws `JavaException`. `DXEndpoint::getRequestedAggregationPeriod()` returns
      the period or `std::nullopt` (`null` in Java). The period can also be set with the
      `DXEndpoint::DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY` property (`"dxfeed.requestedAggregationPeriod"`).
    * The new `TimePeriodInfo` is the range of aggregation periods: `getMin()`, `getMax()`, `isUnknown()`,
      `TimePeriodInfo::UNKNOWN` and `EMPTY`, `valueOf(min, max)`, and `toString()` as in Java (`{"min":1.5,"max":2}`,
      in seconds).
    * `DXEndpoint::getAggregationPeriodInfo()` returns the periods that the servers report over the connections of the
      endpoint, and `DXFeedSubscription::getAggregationPeriodInfo()` the period applied to the delivered events; both
      are `TimePeriodInfo::UNKNOWN` until known.
* **\[MDAPI-457]\[C++]** Added `DXFeedSubscription::getTotalDroppedEvents()` (experimental, as in the Java API): the
  number of events dropped for the subscription since it was created.
* **\[MDAPI-457]\[C++]** Added `Schedule::getDefinition()`: the definition string of the schedule, as accepted by
  `Schedule::getInstance()`.
