* **\[MDAPI-518]\[C++]** **\[BREAKING]** `DXEndpoint`, `DXFeed`, `DXPublisher` and their listeners call the `dxfg_v4_`
  functions of the Graal Native SDK instead of the deprecated `dxfg_` ones.
    * **\[BREAKING]** `DXEndpoint::executor()` is removed: it changed the executor of an endpoint that already existed,
      and the last event promises then never completed. Set the executor with `DXEndpoint::Builder::withExecutor()`
      before `build()`.
