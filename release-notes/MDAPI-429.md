* **\[MDAPI-429]\[C++]** **\[BREAKING]** Fixed `noexcept` functions that threw and the shared library (DLL) on Windows.
    * **\[BREAKING]** `OrderBase::getSource()`, `OrderBase::setExchangeCode(std::int16_t)` and the `withIndex()` /
      `withExchangeCode()` methods of `Order`, `SpreadOrder`, `AnalyticOrder` and `OtcMarketsOrder` are no longer
      `noexcept` (`IndexedEvent::getSource()` too). An invalid source id in the index, an invalid exchange code or a
      negative index now throws `InvalidArgumentException` (as Java throws `IllegalArgumentException`) instead of
      terminating the process.
    * **\[BREAKING]** `CmdArgsUtils::parseEventSources()` is no longer `noexcept`. An invalid source name now throws
      `InvalidArgumentException` instead of terminating the process.
    * `StringLike`, `StringHash`, `EventFlagsMask`, `DXFeedTimeSeriesSubscription` and the `_c` literal are exported
      from the shared library. An application that uses the DLL did not link calls with `const char*` and `std::string`
      arguments, `EventFlagsMask::contains()` and `DXFeedTimeSeriesSubscription::getFromTime()` / `setFromTime()`.
    * Fixed the shared library on Windows: the subscriptions and the listeners of the tx models that the code of the
      public headers creates were registered in managers of the application instead of the library, and the ids of the
      listeners added by the application and by the library collided, so these listeners were not called. The managers
      are kept in the library; the listener ids are unique within a handler (`Handler`, `SimpleHandler`).
