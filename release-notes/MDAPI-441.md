* **\[MDAPI-441]\[C++]** **\[BREAKING]** The listeners are called on the notifying thread (the executor thread of the
  dxFeed Graal SDK, as in the Java API), not on a new thread for each notification, and no internal lock is held while
  they run.
    * `SimpleHandler` calls the listeners from a snapshot of the list: a listener can remove itself, add a listener
      (called from the next notification) and notify the same handler. If the source notifies on several threads at
      once, the listeners are called on these threads at once.
    * `SimpleHandler::remove()` returns after the call of that listener in progress on another thread ends; called from
      a listener or a callback of the library, it does not wait. `MarketDepthModel::close()` waits the same way.
    * An exception of a listener goes to `ErrorManager`, and the other listeners of the batch are still called. It was
      lost before, together with the rest of the batch.
    * `InstrumentProfileCollector` and `DXPublisherObservableSubscription` call the listeners and the SDK without
      holding their locks: `removeUpdateListener()` from a listener of the collector no longer hangs.
    * Removed the unused `Handler` (`internal/Handler.hpp`). Use `SimpleHandler`.
