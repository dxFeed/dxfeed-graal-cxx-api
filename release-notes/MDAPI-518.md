* **\[MDAPI-518]\[C++]** **\[BREAKING]** `DXEndpoint`, `DXFeed`, `DXPublisher`, `DXFeedSubscription`,
  `DXFeedTimeSeriesSubscription`, `SubscriptionController` and their listeners call the `dxfg_v4_` functions of the
  Graal Native SDK instead of the deprecated `dxfg_` ones: the library calls no deprecated function of the SDK.
    * **\[BREAKING]** `DXEndpoint::executor()` is removed: it changed the executor of an endpoint that already existed,
      and the last event promises then never completed. Set the executor with `DXEndpoint::Builder::withExecutor()`
      before `build()`.
    * **\[BREAKING]** `JavaException::throwIfNullptr()`, `nullableOrThrow()`, `throwIfLessThanZero()`, `isMinusOne()`,
      `throwIfMinusOne()`, `throwIfMinusMin()` and `throwIfMinusInf()` are removed: they checked the results of the
      deprecated functions of the SDK.
    * `SubscriptionController` and `DXFeedSubscription::getTotalDroppedEvents()` use experimental functions of the
      SDK: they throw `RuntimeException` with both revisions when the SDK library has another advanced revision than
      the one the library is built for.
    * Building from the sources with GCC or Clang: a call of a deprecated function of the SDK is an error
      (`-Werror=deprecated-declarations`) in the code of this project; the projects that use the library do not get
      the option.
