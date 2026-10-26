* **\[MDAPI-451]\[C++]** Added the constants and helpers of QDS 3.330–3.351 that the Java API has.
    * `InstrumentProfileType::CERTIFICATE`. It follows `WARRANT` in `InstrumentProfileType::ALL`, as in Java, but has the
      next free code, so the codes of the other types do not change.
    * `Schedule::UTC()` (a 5-day week in UTC with no holidays) and `Schedule::NYSE()`: the schedules of the Java
      `Schedule.UTC` and `Schedule.NYSE`, created at the first call.
    * The property keys `DXEndpoint::DXFEED_STRIPE_PROPERTY` (`dxfeed.stripe`),
      `DXFEED_STICKY_SUBSCRIPTION_PROPERTY` (`dxfeed.stickySubscriptionPeriod`) and
      `DXFEED_REQUESTED_AGGREGATION_PERIOD_PROPERTY` (`dxfeed.requestedAggregationPeriod`).
    * `MarketEventSymbols::hasAttributes()`, `buildSpreadSymbol()`, `getExchangesByPattern()`, `EXCHANGES_PATTERN` and
      `SUPPORTED_EXCHANGES`. `getExchangesByPattern()` takes an ECMAScript character class (`std::regex`), not the Java
      regular expression syntax.
    * `DXFeedSubscription::OPTIMAL_BATCH_LIMIT` and `MAX_BATCH_LIMIT` are `static constexpr`, so code that binds them by
      reference (`std::max`, the comparison macros of test frameworks) links with the shared library. Their type and
      values do not change.
