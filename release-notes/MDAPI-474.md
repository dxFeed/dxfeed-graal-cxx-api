* **\[MDAPI-474]\[C++]** A subscription can report that its data stopped while the connection stays up.
    * `DXFeedSubscription::setSilenceWatchdog(timeout, onSilence)` turns on a watchdog: when the subscription receives
      no events for `timeout` while the endpoint of its feed is connected, a warning is reported through
      `ErrorManager` with the subscription (its number and event types) and the time of the silence, then `onSilence`
      is called. Once per silence: the next batch of events or a new connection arms it again.
    * The watchdog is off by default: a symbol that does not trade, a quiet market or a subscription without symbols
      is silent too. A subscription that is attached to no feed or whose endpoint is not connected is not watched.
    * `onSilence` is called on one thread of the library for all the subscriptions, outside of its locks, and may
      call the subscription and the endpoint; the documentation shows a reconnect of the endpoint from it.
    * Fixed: after an `IllegalStateException` of `addEventListener()` (a subscription without event listeners that is
      attached and has symbols), a later call no longer takes the listener for added without delivering events.
