* **\[MDAPI-442]\[C++]** Closed subscriptions and endpoints are released, as in the Java API: an application that
  creates and closes them no longer grows in memory, and the listeners of a subscription are not called after
  `close()` returns.
    * The code that uses the API stays the same: only the behaviour after `close()` changes.
    * An attached subscription is owned by its feed until it is closed or detached, or the endpoint is closed: a
      subscription without a pointer of the user receives events until then. A subscription without a feed is closed
      with its last pointer.
    * A created endpoint lives until `close()` or `closeAndAwaitTermination()`; then it is released with its feed and
      publisher when the user releases it. The default endpoints stay for the whole process.
    * `DXFeedSubscription::close()` returns after the calls of its listeners in progress on other threads end; called
      from a listener, it does not wait. A subscription or an endpoint can be closed from a listener of a subscription.
    * `CLOSED` is the last state notification of an endpoint. A closed subscription delivers no events; adding a
      listener to it, attaching it to a feed or closing it again does nothing.
    * `DXEndpoint::executor()` called after `getFeed()` applies to the next events of the existing subscriptions, as
      in the Java API.
    * `OnDemandService::getInstance(endpoint)` returns the one service of the endpoint, as in the Java API; the
      service is released with its endpoint. `getEndpoint()` of a service kept longer returns an empty pointer.
    * Transaction models, `InstrumentProfileCollector`, `InstrumentProfileReader` and `HistoryEndpoint` are released
      with their last pointer. A model owns its listener; an `InstrumentProfileConnection` owns its collector.
    * New `AutoClose<T>` closes a subscription, an endpoint or another entity with `close()` at the end of the scope,
      as try-with-resources does in Java. The documentation page "Lifetime of the entities" describes the owners and
      the guarantees of `close()`.
    * The ids of the entities are unique when the library is linked dynamically: a subscription created from a header
      template and one created in the library could get the same id, and the events of the first went to the second.
