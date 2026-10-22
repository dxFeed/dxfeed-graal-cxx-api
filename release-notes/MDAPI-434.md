* **\[MDAPI-434]\[C++]** Fixed `MarketDepthModel` with a nonzero aggregation period: `close()` now cancels the
  pending notification, the listener is not called after `close()` or after the model is destroyed, and the listener
  can call the model (`close()` included) without a deadlock. `Timer` no longer leaks itself and the state of its
  task (each aggregated notification leaked one): the task runs on a detached thread.
  [#124](https://github.com/dxFeed/dxfeed-graal-cxx-api/issues/124)
