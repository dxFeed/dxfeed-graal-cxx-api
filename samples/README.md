# dxFeed Graal CXX API Samples

## Useful addresses

* "demo.dxfeed.com:7300"
  Address of free dxFeed demo feed with some test symbols that tick 24/7 and select delayed data.
* "ondemand:demo.dxfeed.com:7680"
  Address of free dxFeed on-demand data replay service feed that gives full historically access to
  data
* "http://dxfeed.s3.amazonaws.com/masterdata/ipf/demo/mux-demo.ipf.zip"
     URL for instrument profiles for everything provided in demo feed.

The username for free demo services is "demo" and password is "demo".
  Access to the free demo services is configured by default in the provided `dxfeed.system.properties` file.

## DXFeedTimeAndSalesSample

The cross-platform UI sample in `cpp/UI/DXFeedTimeAndSalesSample` uses Dear ImGui, GLFW, and OpenGL. CMake uses the
pinned UI sources from `third_party` when they are present (for example, in the Full Source Bundle) and downloads them
otherwise. The data endpoint is selected through the sample's
`dxfeed.system.properties` file, which is copied next to the executable.

Build and run the `DXFeedTimeAndSalesSample` target, enter a symbol such as `AAPL`, then press Enter or **Subscribe**.
The table keeps the latest 30 trades. Feed callbacks accumulate `IndexedTxModel` transactions in a synchronized
mailbox; the window reads the newest immutable view on each frame, so dxFeed does not need a UI-specific executor.

On Linux, the sample builds GLFW with the X11 backend by default, which also works under WSLg through XWayland. Native
Wayland support can be enabled with `-DDXFCXX_SAMPLE_GLFW_BUILD_WAYLAND=ON` when `wayland-scanner` and the Wayland
development packages are installed.

The snapshot accumulator can be tested without a display or network connection:

```shell
ctest --test-dir <build-directory> -R DXFeedTimeAndSalesStoreTest --output-on-failure
```

For an integration smoke test, run the application against the demo endpoint, subscribe to `AAPL`, verify that the
description appears and the table stays at 30 rows or fewer, then switch symbols and verify that old rows are cleared.
