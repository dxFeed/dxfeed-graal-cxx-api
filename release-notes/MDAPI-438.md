* **\[MDAPI-438]\[C++]** Migrated to Graal SDK v3.9.0 (QDS 3.356).
    * Added `DXEndpoint::Builder::withExecutor()`: it sets the executor of the endpoint's notifications before the
      endpoint is built. `DXEndpoint::executor()` still works but is deprecated: called after `DXEndpoint::getFeed()`
      or `DXEndpoint::getPublisher()`, it leaves the feed with the default executor, which is shut down, and the last
      event promises (`DXFeed::getLastEventPromise()`, etc.) never complete (QD-1795). Until the migration, call it
      right after the endpoint is created. Its first call in a process logs a deprecation warning of the SDK.
    * QD: absolute Windows paths are accepted in `tape:` addresses (QD-1796); concurrent creation of endpoints with
      the `ext` scheme no longer fails (QD-1794); `MarketMaker` can be written to XML (QD-1799).
    * QD, incompatible: the `TimeAndSale` time in the XML format of tapes is an ISO 8601 date and time
      (`2025-10-02T09:00:00.000Z`) instead of milliseconds, as in the other events (QD-1798).
