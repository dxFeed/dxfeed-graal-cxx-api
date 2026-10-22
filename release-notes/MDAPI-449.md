* **\[MDAPI-449]\[C++]** Added `IndexedEventTxModel` (the incremental model of indexed events of dxFeed Java API) with
  its builder (feed, symbol, source, listener, events batch limit, aggregation period, executor), and
  `SubscriptionController` (attach, detach, events batch limit, aggregation period, executor) that
  `IndexedEventTxModel::getSubscriptionController()` returns. Added `SimpleOrderBookSample`.
