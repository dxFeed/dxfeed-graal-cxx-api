* **\[MDAPI-471]\[C++]** The first endpoint of the process reports the versions, and `DXEndpoint::connect` rejects
  the URLs that are not addresses of an endpoint.
    * The first endpoint created in the process reports one `INFO` line through `ErrorManager` with the versions of
      the C++ API and of the Graal Native SDK and the platform; the default handler prints it to `stderr`:
      `[dxfcpp] INFO #1 DXEndpoint: dxFeed Graal C++ API v9.0.0, dxFeed Graal Native SDK 3.16.0, Windows x86_64(16 cores)`
      (`ErrorInfo::toString()` gives no location for an `INFO` message). The version of QD is in the log of the SDK (`Using QDS-<version>, (C) Devexperts`).
    * The platform of macOS is named `MacOS`; it was `iOS|tvOS|watchOS-Simulator` (also in the output of the
      `PerfTest`, `PerfTest2` and `LatencyTest` tools).
    * `ErrorInfo::Level` has a new value `INFO` after `WARN`; only the library reports it. A custom handler and the
      journal get the `INFO` line too: a handler that treats every report as an error should check `level`.
    * **\[BREAKING]** `DXEndpoint::connect` with an address that starts with `http://`, `https://`, `ws://` or `wss://`
      throws `InvalidArgumentException` instead of `JavaException`. The message names the address form
      `host:port[login=...]`, and `dxlink:wss://...` for a WebSocket URL; such addresses never worked. A QD
      configuration URL (`https://host/path/name.config#name`) and the other forms are passed to the SDK as before.
      `dxfc_dxendpoint_connect` returns `DXFC_EC_ERROR` for them, as before.
