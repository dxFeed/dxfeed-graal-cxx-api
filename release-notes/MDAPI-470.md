* **\[MDAPI-470]\[C++]** The timeout exception of a promise from `DXFeed::getLastEventPromise`,
  `getLastEventsPromises`, `getIndexedEventsPromise` and `getTimeSeriesPromise` tells what was requested, how long the
  wait took and the state of the endpoint.
    * `Promise::await(timeout)` still throws the same `JavaException` (`java.util.concurrent.CancellationException`);
      its message gets the method, the event type, the symbol, the source or the time range, the elapsed time, the
      timeout and the endpoint state (`unknown` when the endpoint is gone).
    * `TimeSeriesTxModel::Builder::withFromTime` and `TimeSeriesTxModel::setFromTime` report a warning through
      `ErrorManager` when `fromTime` is now or in the future; the value is still set.
    * `ErrorInfo` has a new last field `level` (`ErrorInfo::Level::ERROR` by default, `WARN` for warnings), and
      `ErrorManager::reportWarning` reports a warning; the default handler prints `WARN` before its number.
      A custom handler and the journal get warnings too: a handler that treats every report as an error should
      check `level`.
