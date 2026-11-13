* **\[MDAPI-519]\[C++]** `dxfcpp::shutdown()` stops the listeners with `dxfg_v4_system_shutdown` of the Graal Native
  SDK: after it returns, no listener of the endpoints, the subscriptions, the models and the instrument profile
  connections is called, and none of their calls is in progress, however long the calls take.
    * `dxfcpp::shutdown()` called from a listener or a callback of the library throws `RuntimeException` and stops
      nothing: it would wait for the call it is made from.
