* **\[MDAPI-642]\[C++]** `DXPublisher::publishEvents` converts a batch of events to the native list in one arena:
  a few heap allocations per batch instead of a few per event, about 50 ns per event instead of 125 ns.
    * Added `EventMapper::toGraalListInArenaUnique` and `EventMapper::freeGraalListInArena`.
