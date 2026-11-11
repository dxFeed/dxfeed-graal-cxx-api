* **\[MDAPI-516]\[C++]** The common types and the leaf areas call the `dxfg_v4_` functions of the Graal Native SDK
  instead of the deprecated `dxfg_` ones.
    * Strings, lists, `TimeFormat`, `TimePeriod`, event types, `InPlaceExecutor`, the system properties and the parsing
      of symbol lists.
    * `AuthToken`, `CFI`, `PriceIncrements` and `AdditionalUnderlyings`.
    * `IndexedTxModel::getSources` no longer leaks the array of the sources that the SDK returns.
