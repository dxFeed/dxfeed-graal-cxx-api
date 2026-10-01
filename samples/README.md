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

The sample is built when its dependencies are found (on Linux, the OpenGL and the X11 or Wayland development
packages); see `DXFCXX_BUILD_UI_SAMPLES` in [How To Build](../HOW_TO_BUILD.md#ui-samples).

Build and run the `DXFeedTimeAndSalesSample` target, enter a symbol such as `AAPL`, then press Enter or **Subscribe**.
The table keeps the latest 30 trades. Feed callbacks accumulate `IndexedTxModel` transactions in a synchronized
mailbox; the window reads the newest immutable view on each frame, so dxFeed does not need a UI-specific executor.
Regional symbols are supported as well: for `AAPL&Q`, TimeAndSale remains subscribed to `AAPL&Q`, while Profile uses
the composite symbol `AAPL` so that the instrument description is still available.

On Linux, the sample builds GLFW with every backend whose development packages are installed, X11, Wayland or both,
and selects one when the window opens: the one of `XDG_SESSION_TYPE` (`wayland` or `x11`); without it (for example,
under WSLg) X11 when `DISPLAY` is set, otherwise Wayland. The chosen one is printed as `GLFW platform: ...`.
`-DDXFCXX_SAMPLE_GLFW_BUILD_WAYLAND=ON` makes the Wayland backend required.

On Wayland, the window has a title bar and buttons when the compositor draws them (KDE) or when libdecor is installed
(GNOME, WSLg): `sudo apt-get install libdecor-0-plugin-1-gtk`. Otherwise GLFW draws only plain borders.

The snapshot accumulator can be tested without a display or network connection:

```shell
ctest --test-dir <build-directory> -R DXFeedTimeAndSalesStoreTest --output-on-failure
```

For an integration smoke test, run the application against the demo endpoint, subscribe to `AAPL`, verify that the
description appears and the table stays at 30 rows or fewer, then switch to `AAPL&Q`. Verify that regional trades are
shown, the composite `AAPL` profile still supplies the description, and rows from the previous subscription are
cleared.
